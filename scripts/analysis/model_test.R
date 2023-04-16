library(tidymodels)
library(ggplot2)
library(corrr)

source("model.R")
source("plots.R")

## Whether to use bootstrapping or cross-validation for resamples
bootstrap <- FALSE
training_proportion <- 0.75
num_resamples <- 3

raw_dataset <- processed_acs_dataset("../../config.yaml")

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
    resamples_from_train <- vfold_cv(train, v = num_resamples)
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

bleeding_fits <- fit_models_to_resamples(workflow, resamples_from_train)

bleeding_fits %>%
    overall_resample_metrics()

bleeding_resample_workflows <- trained_resample_workflows(bleeding_fits)

predictions_for_resamples <-
    model_predictions_for_resamples(bleeding_resample_workflows, test)

bleeding_models_aucs <- predictions_for_resamples %>%
    resample_model_aucs(bleeding_after, .pred_bleeding_occurred)

bleeding_model_resample_roc_curves <- predictions_for_resamples %>%
    resample_model_roc_curves(bleeding_after, .pred_bleeding_occurred)

bleeding_model_resample_roc_curves %>%
    plot_resample_roc_curves()

