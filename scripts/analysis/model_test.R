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

##' A decision tree along with a specification for
##' tuning its hyper-parameters
make_decision_tree <- function() {
    list (
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

##' A logistic regression model (no hyper-parameters)
make_logistic_regression <- function() {
    list (
        model = logistic_reg() %>% 
            set_engine('glm') %>% 
            set_mode('classification')
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

logistic_regression_results = make_logistic_regression() %>%
    optimal_workflow(recipe, train) %>%
    fit_model_on_bootstrap_resamples(train, test, bleeding_after, "bleeding",
                                     num_bootstrap_resamples)
    

decision_tree_model = make_decision_tree() %>%
    optimal_workflow(recipe, train) %>%
    fit_model_on_bootstrap_resamples(train, test, bleeding_after, "bleeding",
                                     num_bootstrap_resamples)

bleeding_resample_results
