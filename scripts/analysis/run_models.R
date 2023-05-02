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
num_cross_validation_folds <- 10
num_bootstrap_resamples <- 10

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
naive_bayes_model <-
    make_naive_bayes(num_cross_validation_folds)
decision_tree_model <-
    make_decision_tree(num_cross_validation_folds)
boosted_tree_model <-
    make_boosted_tree(num_cross_validation_folds)
## random_forest_model <-
##     make_random_forest(num_cross_validation_folds)
linear_discriminant_analysis_model <-
    make_linear_discriminant_analysis(num_cross_validation_folds)
quadratic_discriminant_analysis_model <-
    make_quadratic_discriminant_analysis(num_cross_validation_folds)

## Optimal model selection and bootstrap verification
logistic_regression_results <- logistic_regression_model %>%
    model_results(bleeding_recipe, ischaemia_recipe)

naive_bayes_results <- naive_bayes_model %>%
    model_results(bleeding_recipe, ischaemia_recipe)

decision_tree_results <- decision_tree_model %>%
    model_results(bleeding_recipe, ischaemia_recipe)

boosted_tree_results <- boosted_tree_model %>%
    model_results(bleeding_recipe, ischaemia_recipe)

## random_forest_results <- random_forest_model %>%
##     model_results(bleeding_recipe, ischaemia_recipe)

linear_discriminant_analysis_results <- linear_discriminant_analysis_model %>%
    model_results(bleeding_recipe, ischaemia_recipe)

quadratic_discriminant_analysis_results <- quadratic_discriminant_analysis_model %>%
    model_results(bleeding_recipe, ischaemia_recipe)


all_model_predictions <- logistic_regression_results$predictions %>%
    bind_rows(naive_bayes_results$predictions) %>%
    bind_rows(decision_tree_results$predictions) %>%
    bind_rows(boosted_tree_results$predictions) %>%
    ##bind_rows(random_forest_results$predictions) %>%
    bind_rows(linear_discriminant_analysis_results$predictions) %>%
    bind_rows(quadratic_discriminant_analysis_results$predictions)

## The summary table of model AUCs
all_model_auc_summary <- model_aucs(logistic_regression_results,
                                    "Logistic Regression") %>%
    bind_rows(model_aucs(naive_bayes_results,
                         "Naive Bayes")) %>%
    bind_rows(model_aucs(decision_tree_results,
                         "Decision Tree")) %>%
    bind_rows(model_aucs(boosted_tree_results,
                         "Boosted Tree")) %>%
    ## bind_rows(model_aucs(random_forest_results,
    ##                      "Random Forest")) %>%
    bind_rows(model_aucs(linear_discriminant_analysis_results,
                         "Linear Discriminant Analysis")) %>%
    bind_rows(model_aucs(quadratic_discriminant_analysis_results,
                         "Quadratic Discriminant Analysis")) %>%
    summarise_model_aucs() %>%
    mutate(across(-model, signif, 2))
    
