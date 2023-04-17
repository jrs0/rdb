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

bleeding_resample_results <- train %>%
    make_recipe(bleeding_after, ischaemia_after) %>%
    fit_model_on_resamples(model, train, test, resamples_from_train,
                           bleeding_after, "bleeding")


ischaemia_resample_results <- train %>%
    make_recipe(ischaemia_after, bleeding_after) %>%
    fit_model_on_resamples(model, train, test, resamples_from_train,
                           ischaemia_after, "ischaemia")
    
