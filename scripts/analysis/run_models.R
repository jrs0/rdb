library(tidymodels)
library(ggplot2)
library(corrr)

devtools::load_all("../../")

source("dataset.R")
source("model.R")
source("plots.R")
source("descriptive.R")

## Configure global options
training_proportion <- 0.75
num_cross_validation_folds <- 2
num_bootstrap_resamples <- 2

## Load the raw data from file or database
raw_dataset <- processed_acs_dataset("../../config.yaml")

## Process the dataset ready for modelling
dataset <- raw_dataset %>%
    modelling_dataset()

## Make test/train split
split <- dataset %>%
    initial_split(prop = training_proportion, strata = bleeding_after)
train <- training(split)
test <- testing(split)

## Preprocessing specification
bleeding_recipe <- train %>%
    make_recipe(bleeding_after, ischaemia_after)
ischaemia_recipe <- train %>%
    make_recipe(ischaemia_after, bleeding_after)

## Model construction
logistic_regression_model <-
    make_logistic_regression()
decision_tree_model <-
    make_decision_tree(num_cross_validation_folds)
linear_discriminant_analysis_model <-
    make_linear_discriminant_analysis(num_cross_validation_folds)

## naive_bayes_model <-
##     make_naive_bayes(num_cross_validation_folds)

## Optimal model selection and bootstrap verification
logistic_regression_results <- logistic_regression_model %>%
    model_results(bleeding_recipe, ischaemia_recipe)

decision_tree_results <- decision_tree_model %>%
    model_results(bleeding_recipe, ischaemia_recipe)

linear_discriminant_analysis_results <- linear_discriminant_analysis_model %>%
    model_results(bleeding_recipe, ischaemia_recipe)

## naive_bayes_results <- naive_bayes_model %>%
##     model_results(bleeding_recipe, ischaemia_recipe)

## The summary table of model AUCs
all_model_auc_summary <- model_aucs(logistic_regression_results,
                             "Logistic Regression") %>%
    bind_rows(model_aucs(linear_discriminant_analysis_results,
                         "Linear Discriminant Analysis")) %>%
    bind_rows(model_aucs(decision_tree_results,
                         "Decision Tree")) %>%
    summarise_model_aucs() %>%
    mutate(across(-model, signif, 2))
    
