library(tidymodels)
library(ggplot2)
library(corrr)

source("model.R")
source("plots.R")

training_proportion <- 0.75
num_cross_validation_folds <- 5
num_bootstrap_resamples <- 6

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

recipe <- train %>%
    make_recipe(bleeding_after, ischaemia_after)

model <- decision_tree(
    tree_depth = tune(),
    cost_complexity = tune()
) %>% 
    set_engine("rpart") %>%
    set_mode("classification")

tuning_grid <- grid_regular(cost_complexity(),
                            tree_depth(),
                            levels = 5)


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

model_workflow <- optimal_workflow_from_tuning(model, recipe, train, 
                                               tuning_grid, num_cross_validation_folds)

##' Use this function for a model with no hyper-parameters. The
##' optimal workflow is obtained by combining the recipe with the
##' model.
optimal_workflow_no_tuning <- function(model, recipe, train) {
    workflow() %>%
        add_model(model) %>%
        add_recipe(recipe)
}

model <- logistic_reg() %>% 
    set_engine('glm') %>% 
    set_mode('classification')

model_workflow <- optimal_workflow_no_tuning(model, recipe, train)



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
    
