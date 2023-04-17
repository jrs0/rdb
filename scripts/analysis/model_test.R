library(tidymodels)
library(ggplot2)
library(corrr)

source("dataset.R")
source("model.R")
source("plots.R")

training_proportion <- 0.75
num_cross_validation_folds <- 5
num_bootstrap_resamples <- 6

raw_dataset <- processed_acs_dataset("../../config.yaml")

dataset <- raw_dataset %>%
    modelling_dataset()

split <- dataset %>%
    initial_split(prop = training_proportion, strata = bleeding_after)
train <- training(split)
test <- testing(split)


bleeding_recipe <- train %>%
    make_recipe(bleeding_after, ischaemia_after)
ischaemia_recipe <- train %>%
    make_recipe(ischaemia_after, bleeding_after)


logistic_regression_model <- make_logistic_regression()
decision_tree_model <- make_decision_tree()






bootstrap_fit_results <- function(model, recipe, outcome_column, outcome_name) {
    model %>%
        optimal_workflow(recipe, train) %>%
        fit_model_on_bootstrap_resamples(train, test, {{ outcome_column }},
                                         outcome_name,
                                         num_bootstrap_resamples)
}

bind_model_results <- function(a, b) {
    predictions <- a$predictions %>%
        bind_rows(b$predictions)

    model_aucs <- a$model_aucs %>%
        bind_rows(b$model_aucs)

    roc_curves <- a$roc_curves %>%
        bind_rows(b$roc_curves)

    list (
        predictions = predictions,
        model_aucs = model_aucs,
        roc_curves = roc_curves
    )
}

model_results <- function(model) {

    bleeding_results <- model %>%
        bootstrap_fit_results(bleeding_recipe, bleeding_after, "bleeding")
    
    ischaemia_results <- model %>%
        bootstrap_fit_results(ischaemia_recipe, ischaemia_after, "ischaemia")
    
    bind_model_results(bleeding_results, ischaemia_results)
}


logistic_regression_results <- logistic_regression_model %>%
    model_results()


logistic_regression_results$roc_curves %>%
    plot_resample_roc_curves()

logistic_regression_results = make_logistic_regression() %>%
    optimal_workflow(ischaemia_recipe, train) %>%
    fit_model_on_bootstrap_resamples(train, test, ischaemia_after, "ischaemia",
                                     num_bootstrap_resamples)












decision_tree_results = make_decision_tree() %>%
    optimal_workflow(preprocessing_recipe, train) %>%
    fit_model_on_bootstrap_resamples(train, test, bleeding_after, "bleeding",
                                     num_bootstrap_resamples)
