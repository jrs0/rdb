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
    count_to_two_level_factor(bleeding_after, "bleeding") %>%
    count_to_two_level_factor(ischaemia_after, "ischaemia") %>%
    remove_other_outcome_columns() %>%
    remove_mortality_columns() %>%
    drop_na() %>%
    mutate(index_id = as.factor(row_number()))

split <- initial_split(dataset, prop = training_proportion,
                       strata = bleeding_after)
train <- training(split)
test <- testing(split)

if (bootstrap) {
    resamples_from_train <- bootstraps(train, times = num_resamples)
} else {
    resamples_from_train <- vfold_cv(train, v = num_resamples)
}

bleeding_recipe <- train %>%
    make_recipe(bleeding_after, ischaemia_after)

logistic_regression_model <- logistic_reg() %>% 
    set_engine('glm') %>% 
    set_mode('classification')

workflow <- workflow() %>%
    add_model(logistic_regression_model) %>%
    add_recipe(bleeding_recipe)

bleeding_fits <- workflow %>%
    fit_models_to_resamples(resamples_from_train)

bleeding_fits %>%
    overall_resample_metrics()

bleeding_resample_workflows <- bleeding_fits %>%
    trained_resample_workflows()

predictions_for_resamples <- bleeding_resample_workflows %>%
    model_predictions_for_resamples(test)

predictions_for_resamples %>%
    resample_model_aucs(bleeding_after, .pred_bleeding_occurred)

bleeding_model_resample_roc_curves <- predictions_for_resamples %>%
    resample_model_roc_curves(bleeding_after, .pred_bleeding_occurred)

bleeding_model_resample_roc_curves %>%
    plot_resample_roc_curves()

