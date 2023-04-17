library(tidymodels)
library(tidyverse)


##' Convert a factor column which two levels to a numeric
##' column containing 0/1. Specify the factor level corresponding
##' to one as an argument.
two_level_factor_to_numeric <- function(dataset, factor_column, one_level) {
    factor_levels <- dataset %>%
        pull({{ factor_column }}) %>%
        levels()
    if (length(factor_levels) != 2) {
        stop("Factor must have exactly two levels")
    }
    if (!(one_level %in% factor_levels)) {
        stop("Required level '", one_level, "' not present in factor")
    }
    dataset %>%
        mutate({{ factor_column }} :=
                   if_else({{ factor_column }} == one_level, 1, 0))
}

##' Convert a column of counts to a factor representing count != 0
##' (event_name_occurred) or count == 0 (non_event_name). Modifies the
##' column in place.
count_to_two_level_factor <- function(dataset, count_column, event_name) {
    dataset %>%
        mutate({{ count_column }} := factor({{ count_column }} == 0,
                                            labels = c("occurred", "none")))
}

#' Only keep bleeding_after and ischaemia_after, drop all other
#' *_after columns
remove_other_outcome_columns <- function(dataset) {
    dataset %>%
        dplyr::select(- (matches("after") &
                         !matches("(bleeding_after|ischaemia_after)")))
}

remove_mortality_columns <- function(dataset) {
    dataset %>%
        dplyr::select(- c(survival_time, cause_of_death))
}

##' Expects a dataset as input containing a set of columns to ignore
##' (index_id, nhs_number, index_date), two outcome columns, one to model
##' and one to ignore, and all other columns are predictors. Returns a recipe
##' that drops near-zero-variance columns, and centers and scales all
##' numeric predictors.
make_recipe <- function(train, outcome_to_model, outcome_to_ignore) {
    recipe(train) %>%
        update_role(everything(), new_role = "predictor") %>%
        update_role({{ outcome_to_model }}, new_role = "outcome") %>%
        update_role({{ outcome_to_ignore }}, new_role = "ignored_outcome") %>%
        update_role(nhs_number, new_role = "nhs_number") %>%
        update_role(index_date, new_role = "index_date") %>%
        update_role(index_id, new_role = "index_id") %>%
        step_nzv(all_predictors()) %>%
        step_normalize(all_numeric_predictors())
}

##' Requires exactly one NZV (near-zero variance) step
##' @param recipe The recipe containing the step_nzv()
##' @return A character vector of predictor column names
##' that are removed due to NZV
near_zero_variance_columns <- function(recipe) {
    nzv_row_numbers <- recipe %>%
        prep() %>%
        tidy() %>%
        filter(type == "nzv")

    if (nrow(nzv_row_numbers) != 1) {
        stop("There must be exactly one NZV predictor in the recipe")
    }
    
    recipe %>%
        prep() %>%
        tidy(number = nzv_row_numbers[[1]]) %>%
        pull(terms)
}

##' Preprocess data using a recipe
##' @param recipe The recipe defining the training data
##' preprocessing steps
##' @return A tibble with the preprocessed results
preprocess_data <- function(recipe) {
    recipe %>%
        prep() %>%
        juice()
}

##' This function performs the step_nzv(), but not any of the
##' others, so that you can see the original data from the columns
##' that will be included in the model.
##' 
##' @param recipe The recipe defining the preprocessing steps
##' @return The dataset without the near-zero variance columns
##' 
without_near_zero_variance_columns <- function(recipe, dataset) {
    nzv_cols <- near_zero_variance_columns(recipe)
    retained_cols <- recipe %>%
        summary() %>%
        filter(!(variable %in% nzv_cols)) %>%
        pull(variable)
    dataset %>%
        select(retained_cols)
}

##' Do the fits for each resample. This it to assess the variability
##' in model parameters as a function of variability in the dataset
##' @param workflow The workflow with the model and recipe
##' @param resamples The resamples to fit
fit_models_to_resamples <- function(workflow, resamples) {
    ## You appear to need to tell it to extract something. THe
    ## identity function is needed because x is everything (I think),
    ## so it is just telling the function to save the whole internal fit
    ## fit.
    ctrl_rs <- control_resamples(extract = identity)
    workflow %>%
        fit_resamples(resamples, control = ctrl_rs)
}

##' Get the overall (averaged) metrics over the resamples.
##' @param fits The output from fit_models_to_resamples()
overall_resample_metrics <- function(fits) {
    fits %>%
        collect_metrics()
}

##' Get the models from the fits to the resamples
##' @param fits The result of a call to fit_models_to_resamples()
trained_resample_workflows <- function(fits) {
    fits %>%
        pull(.extracts) %>%
        list_rbind() %>%
        pull(.extracts)
}

##' Use each model (one per resample) to make predictions on the test
##' set. Return the result in a tibble with an id specifying which
##' resample made the predictions. The returned data is the test data
##' augmented by the predictions.
model_predictions_for_resamples <- function(models, test) {
    list(
        m = models,
        n = seq_along(models)
    ) %>%
        pmap(function(m, n) {
            m %>%
                augment(test) %>%
                mutate(resample_id = as.factor(n))
        }) %>%
        list_rbind()

    ## If you need it wide: 
    ## pivot_wider(names_from = resample_id,
    ##             names_prefix = "resample_",
    ##             values_from = matches(".pred"))
}

##' A tibble of the AUCs for the resample models
resample_model_aucs <- function(predictions, truth, probability) {
    predictions %>%
        group_by(resample_id) %>%
        roc_auc({{ truth }}, {{ probability }})
}

##' The test data resample predictions tibble, augmented by
##' ROC curves (sensitivity and selectivity columns)
resample_model_roc_curves <- function(predictions, truth, probability) {
    predictions %>%
        group_by(resample_id) %>%
        roc_curve({{ truth }}, {{ probability }})
}

##' Fit a particular model on a set of resamples of the training data,
##' test the models on the test data, and return the AUC and ROC curves
##' for the models.
fit_model_on_resamples <- function(recipe, model, test, resamples,
                                   outcome_column, outcome_name) {
    results = list()
    
    model_workflow <- workflow() %>%
        add_model(model) %>%
        add_recipe(recipe)

    fits <- model_workflow %>%
        fit_models_to_resamples(resamples)

    results$overal_metrics <- fits %>%
        overall_resample_metrics()

    predictions_for_resamples <- fits %>%
        trained_resample_workflows() %>%
        model_predictions_for_resamples(test) %>%
        mutate(outcome = outcome_name)

    results$model_aucs <- predictions_for_resamples %>%
        resample_model_aucs({{ outcome_column }}, .pred_occurred)

    results$roc_curves <- predictions_for_resamples %>%
        resample_model_roc_curves({{ outcome_column }}, .pred_occurred) %>%
        plot_resample_roc_curves()

    results
}
