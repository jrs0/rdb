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

## model <- logistic_reg() %>% 
##     set_engine('glm') %>% 
##     set_mode('classification')

model <- decision_tree(
    tree_depth = tune(),
    cost_complexity = tune()
) %>% 
    set_engine("rpart") %>%
    set_mode("classification")

tuning_grid <- grid_regular(cost_complexity(),
                            tree_depth(),
                            levels = 5)

recipe <- train %>%
    make_recipe(bleeding_after, ischaemia_after)

model_workflow <- workflow() %>%
    add_model(model) %>%
    add_recipe(recipe)

best_model <- model_workflow %>%
    tune_models_with_resamples(resamples_from_train, tuning_grid) %>%
    select_best("roc_auc")

optimal_fit <- model_workflow %>%
    finalize_workflow(best_model)

    

optimal_fit <- model_workflow %>%
    fit(data = train)


fits <- model_workflow %>%
    fit_models_to_resamples(resamples_from_train)



overall_metrics <- fits %>%
    overall_resample_metrics()

predictions_for_resamples <- fits %>%
    trained_resample_workflows() %>%
    model_predictions_for_resamples(test) %>%
    mutate(outcome = outcome_name)

model_aucs <- predictions_for_resamples %>%
    resample_model_aucs({{ outcome_column }}, .pred_occurred)

roc_curves <- predictions_for_resamples %>%
    resample_model_roc_curves({{ outcome_column }}, .pred_occurred) %>%
    plot_resample_roc_curves()



ischaemia_resample_results <- train %>%
    make_recipe(ischaemia_after, bleeding_after) %>%
    fit_model_on_resamples(model, train, test, resamples_from_train,
                           ischaemia_after, "ischaemia")
    
