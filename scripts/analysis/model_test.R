library(tidymodels)
library(ggplot2)
library(corrr)

source("model.R")
source("plots.R")

## Whether to use bootstrapping or cross-validation for resamples
bootstrap <- TRUE
training_proportion <- 0.75
num_resamples <- 5

raw_dataset <- processed_acs_dataset("../../config.yaml")

dataset <- raw_dataset %>%
    count_to_two_level_factor(bleeding_after) %>%
    count_to_two_level_factor(ischaemia_after) %>%
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

ischaemia_recipe <- train %>%
    make_recipe(ischaemia_after, bleeding_after)

## logistic_regression_model <- logistic_reg() %>% 
##     set_engine('glm') %>% 
##     set_mode('classification')


fit_model_on_resamples <- function(recipe, model, resamples,
                                   outcome_column, outcome_name) {

    results = list()
    
    model_workflow <- workflow() %>%
        add_model(model) %>%
        add_recipe(bleeding_recipe)

    fits <- model_workflow %>%
        fit_models_to_resamples(resamples)

    results$overal_metrics <- bleeding_fits %>%
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


bleeding_resample_results <-
    fit_model_on_resamples(bleeding_recipe, logistic_regression_model,
                       resamples_from_train, bleeding_after, "bleeding")
    
