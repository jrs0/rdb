library(tidymodels)
library(ggplot2)
library(corrr)

source("plots.R")

## Whether to use bootstrapping or cross-validation for resamples
bootstrap <- TRUE

training_proportion <- 0.75

## The number of resamples in the cross-validation folds/botstrapping
num_resamples <- 3

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

raw_dataset <- processed_acs_dataset("../../config.yaml")

## Simple proportions from the data
dataset <- raw_dataset %>%
    mutate(bleeding_after = factor(bleeding_after == 0, labels = c("bleeding_occurred", "no_bleed"))) %>%
    mutate(ischaemia_after = factor(ischaemia_after == 0, labels = c("ischaemia_occurred", "no_ischaemia"))) %>%
    ## Remove the after columns
    dplyr::select(- (matches("after") & !matches("(bleeding_after|ischaemia_after)"))) %>%
    ## Remove mortality columns
    dplyr::select(- c(survival_time, cause_of_death)) %>%
    ## Now that the mortality columns are gone (the survival time columns contains
    ## NAs when patient is alive), drop other NAs
    drop_na() %>%
    ## Add an ID to link up patients between bleeding and ischaemia predictions
    mutate(index_id = as.factor(row_number()))

split <- initial_split(dataset, prop = training_proportion,
                       strata = bleeding_after)
train <- training(split)
test <- testing(split)


## Create cross-validation folds
if (bootstrap) {
    resamples_from_train <- bootstraps(train, times = num_resamples)
} else {
    resamples_from_train <- vfold_cv(train, times = num_resamples)
}

bleeding_recipe <- recipe(bleeding_after ~ ., data = train) %>%
    update_role(index_id, new_role = "id") %>%
    update_role(nhs_number, new_role = "nhs_number") %>%
    update_role(index_date, new_role = "date") %>%
    update_role(ischaemia_after, new_role = "ischaemic_after") %>%
    step_nzv(all_predictors()) %>%
    step_normalize(all_numeric_predictors())

after_preprocessing <- bleeding_recipe %>%
    preprocess_data()

## This is just for debugging -- other preprocessing not done
after_nzv_removal <- bleeding_recipe %>%
    without_near_zero_variance_columns(dataset)

log_reg <- logistic_reg() %>% 
    set_engine('glm') %>% 
    set_mode('classification')

workflow <- workflow() %>%
    add_model(log_reg) %>%
    add_recipe(bleeding_recipe)

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

bleeding_fits <- fit_models_to_resamples(workflow, resamples_from_train)

##' Get the overall (averaged) metrics over the resamples.
##' @param fits The output from fit_models_to_resamples()
overall_resample_metrics <- function(fits) {
    fits %>%
        collect_metrics()
}

bleeding_fits %>%
    overall_resample_metrics()

##' Get the models from the fits to the resamples
##' @param fits The result of a call to fit_models_to_resamples()
trained_resample_workflows <- function(fits) {
    fits %>%
        pull(.extracts) %>%
        list_rbind() %>%
        pull(.extracts)
}

bleeding_resample_workflows <- trained_resample_workflows(bleeding_fits)

model_predictions_for_resamples <- function(models, test) {
    list(
        m = models,
        n = seq_along(models)
    ) %>%
        pmap(function(m, n) {
            m %>%
                predict(test) %>%
                mutate(resample_id = as.factor(n))
        }) %>%
        list_rbind()
}

predictions_for_resamples <-
    model_predictions_for_resamples(bleeding_resample_workflows, test)



bleeding_models_aucs <- model_aucs(bleeding_models)
bleeding_predictions <- bleeding_fit %>%
    augment(new_data = test) %>%
    select(bleeding_after, .pred_bleeding_occurred, .pred_class)

roc_curves <- bleeding_predictions %>%
    roc_curve(bleeding_after, .pred_bleeding_occurred)

roc_curves %>%
    ggplot(aes(x = 1 - specificity, y = sensitivity)) +
    geom_line() +
    geom_abline(slope = 1, intercept = 0, size = 0.4)
