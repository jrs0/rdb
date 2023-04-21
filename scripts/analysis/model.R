library(tidyverse)
library(tidymodels)
library(themis)

library(discrim)
library(mda)

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
        ##step_rose({{ outcome_to_model }})
        ##step_downsample({{ outcome_to_model }})
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

retained_predictors <- function(recipe) {
    nzv_cols <- near_zero_variance_columns(recipe)
    recipe %>%
        summary() %>%
        filter(!(variable %in% nzv_cols),
               role == "predictor") %>%
        pull(variable)
}

retained_columns <- function(recipe) {
    nzv_cols <- near_zero_variance_columns(recipe)
    recipe %>%
        summary() %>%
        filter(!(variable %in% nzv_cols)) %>%
        pull(variable)
}

##' This function performs the step_nzv(), but not any of the
##' others, so that you can see the original data from the columns
##' that will be included in the model.
##' 
##' @param recipe The recipe defining the preprocessing steps
##' @return The dataset without the near-zero variance columns
##' 
without_near_zero_variance_columns <- function(recipe, dataset) {
    columns <- recipe %>%
        retained_columns()
    dataset %>%
        select(columns)
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

tune_models_with_resamples <- function(workflow, resamples, tuning_grid) {
    ctrl_rs <- control_resamples(extract = identity)
    workflow %>%
        tune_grid(resamples = resamples,
                  grid = tuning_grid,
                  control = ctrl_rs)
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

##' The test data resample predictions tibble, augmented by
##' lift curves
resample_model_lift_curves <- function(predictions, truth, probability) {
    predictions %>%
        group_by(resample_id) %>%
        lift_curve({{ truth }}, {{ probability }})
}

##' The test data resample predictions tibble, augmented by
##' gain curves
resample_model_gain_curves <- function(predictions, truth, probability) {
    predictions %>%
        group_by(resample_id) %>%
        gain_curve({{ truth }}, {{ probability }})
}


##' The test data resample predictions tibble, augmented by
##' gain curves
resample_model_precision_recall_curves <- function(predictions, truth, probability) {
    predictions %>%
        group_by(resample_id) %>%
        pr_curve({{ truth }}, {{ probability }})
}

##' Get the optimal model over a grid of hyper-parameters
##' using cross-validation on the training set. Use this
##' function when the model has hyperparameters
optimal_workflow_from_tuning <- function(model, recipe, train,
                                      tuning_grid, num_folds) {    
    pre_tuning_workflow <- workflow() %>%
        add_model(model) %>%
        add_recipe(recipe)

    resamples <- vfold_cv(train, v = num_folds)
    best_model <- pre_tuning_workflow %>%
        tune_models_with_resamples(resamples, tuning_grid) %>%
        select_best("roc_auc")

    pre_tuning_workflow %>%
        finalize_workflow(best_model)
}

##' Use this function for a model with no hyper-parameters. The
##' optimal workflow is obtained by combining the recipe with the
##' model.
optimal_workflow_no_tuning <- function(model, recipe, train) {
    workflow() %>%
        add_model(model) %>%
        add_recipe(recipe)
}

##' Perform the fit on bootstrap resamples to assess stability in the
##' output probabilities with variations in the training set.
fit_model_on_bootstrap_resamples <- function(model_workflow, train, test,
                                             outcome_column, outcome_name,
                                             num_bootstrap_resamples) {
    
    bootstrap_resamples <- bootstraps(train, times = num_bootstrap_resamples)

    full_train_fit <- model_workflow %>%
        fit(data = train)

    full_train_predictions <- full_train_fit %>%
        augment(test) %>%
        mutate(resample_id = as.factor("full_training_set"))
    
    bootstrap_fits <- model_workflow %>%
        fit_models_to_resamples(bootstrap_resamples)

    bootstrap_predictions <- bootstrap_fits %>%
        trained_resample_workflows() %>%
        model_predictions_for_resamples(test)

    predictions <- full_train_predictions %>%
        bind_rows(bootstrap_predictions) %>%
        mutate(outcome = outcome_name)
    
    model_aucs <- predictions %>%
        resample_model_aucs({{ outcome_column }}, .pred_occurred) %>%
        mutate(outcome = outcome_name)

    roc_curves <- predictions %>%
        resample_model_roc_curves({{ outcome_column }}, .pred_occurred) %>%
        mutate(outcome = outcome_name)

    lift_curves <- predictions %>%
        resample_model_lift_curves({{ outcome_column }}, .pred_occurred) %>%
        mutate(outcome = outcome_name)

    gain_curves <- predictions %>%
        resample_model_gain_curves({{ outcome_column }}, .pred_occurred) %>%
        mutate(outcome = outcome_name)

    precision_recall_curves <- predictions %>%
        resample_model_precision_recall_curves({{ outcome_column }}, .pred_occurred) %>%
        mutate(outcome = outcome_name)
    
    list (
        full_train_fit = full_train_fit,
        bootstrap_fits = bootstrap_fits,
        predictions = predictions,
        model_aucs = model_aucs,
        roc_curves = roc_curves,
        lift_curves = lift_curves,
        gain_curves = gain_curves,
        precision_recall_curves = precision_recall_curves
    )
}

##' A decision tree along with a specification for
##' tuning its hyper-parameters
make_decision_tree <- function(num_cross_validation_folds) {
    list (
        name = "decision_tree",
        model = decision_tree(
            tree_depth = tune(),
            cost_complexity = tune()
        ) %>% 
            set_engine("rpart") %>%
            set_mode("classification"),
        tuning_grid = grid_regular(cost_complexity(),
                                   tree_depth(),
                                   levels = 5),
        num_cross_validation_folds = num_cross_validation_folds
    )
}


make_linear_discriminant_analysis <- function(num_cross_validation_folds) {
    list (
        name = "linear_discriminant_analysis",
        model = discrim_linear(
            mode = "classification",
            penalty = tune()
        ) %>%
            set_engine("mda"),
        tuning_grid = grid_regular(penalty(),
                                   levels = 5),
        num_cross_validation_folds = num_cross_validation_folds
    )
}        
    
make_naive_bayes <- function(num_cross_validation_folds) {
    list (
        name = "naive_bayes",
        model = naive_Bayes(
            mode = "classification",
            smoothness = tune(),
            Laplace = tune(),
            engine = "naivebayes"),
        tuning_grid = grid_regular(smoothness(),
                                   Laplace(),
                                   levels = 5),
        num_cross_validation_folds = num_cross_validation_folds
    )
}

##' A logistic regression model (no hyper-parameters)
make_logistic_regression <- function() {
    list (
        name = "logistic_regression",
        model = logistic_reg() %>% 
            set_engine('glm') %>% 
            set_mode('classification')
    )
}

make_boosted_tree <- function(num_cross_validation_folds) {
    list (
        name = "boosted_tree",
        model = boost_tree(
            mode = "classification",
            mtry = tune(),
            trees = tune(),
        ) %>%
            set_engine("xgboost"),
        tuning_grid = grid_regular(mtry(),
                                   trees(),
                                   levels = 5),
        num_cross_validation_folds = num_cross_validation_folds
    )
}        

make_random_forest <- function(num_cross_validation_folds) {
    list (
        name = "random_forest",
        model = rand_forest(
            mode = "classification",
            mtry = tune(),
            trees = tune()
        ) %>%
            set_engine(engine = "ranger"),
        tuning_grid = grid_regular(mtry(),
                                   trees(),
                                   levels = 5),
        num_cross_validation_folds = num_cross_validation_folds
    )
}

optimal_workflow <- function(model, recipe, train) {
    if (!is.null(model$tuning_grid)) {
        k <- model$num_cross_validation_folds
        optimal_workflow_from_tuning(model$model, recipe, train, 
                                     model$tuning_grid, k)
    } else {
        optimal_workflow_no_tuning(model$model, recipe, train)
    }
}

bootstrap_fit_results <- function(model, recipe, outcome_column, outcome_name) {
    model %>%
        optimal_workflow(recipe, train) %>%
        fit_model_on_bootstrap_resamples(train, test, {{ outcome_column }},
                                         outcome_name,
                                         num_bootstrap_resamples)
}

bind_model_results <- function(a, b, model) {
    predictions <- a$predictions %>%
        bind_rows(b$predictions) %>%
        mutate(model_name = model$name)

    model_aucs <- a$model_aucs %>%
        bind_rows(b$model_aucs)

    roc_curves <- a$roc_curves %>%
        bind_rows(b$roc_curves)

    lift_curves <- a$lift_curves %>%
        bind_rows(b$lift_curves)

    gain_curves <- a$gain_curves %>%
        bind_rows(b$gain_curves)

    precision_recall_curves <- a$precision_recall_curves %>%
        bind_rows(b$precision_recall_curves)
    
    list (
        predictions = predictions,
        model_aucs = model_aucs,
        roc_curves = roc_curves,
        lift_curves = lift_curves,
        gain_curves = gain_curves,
        precision_recall_curves = precision_recall_curves
    )
}

model_results <- function(model, bleeding_recipe, ischaemia_recipe) {
    bleeding_results <- model %>%
        bootstrap_fit_results(bleeding_recipe, bleeding_after, "bleeding")
    
    ischaemia_results <- model %>%
        bootstrap_fit_results(ischaemia_recipe, ischaemia_after, "ischaemia")
    
    bind_model_results(bleeding_results, ischaemia_results, model)
}

