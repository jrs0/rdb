library(tidymodels)

##' Requires exactly one NZV (near-zero variance) step
##' @param recipe The recipe containing the step_nzv()
##' @return A character vector of predictor column names
##' that are removed due to NZV
near_zero_variance_predictors <- function(recipe) {
    nzv_row_numbers <- bleeding_recipe %>%
        prep() %>%
        tidy() %>%
        filter(type == "nzv")

    if (length(nzv_row_number) != 1) {
        stop("There must be exactly one NZV predictor in the recipe")
    }
    bleeding_recipe %>%
        prep() %>%
        tidy(number = nzv_row_numbers[[1]])
}

##' Preprocess data using a recipe
##' @param recipe The recipe defining the training data
##' preprocessing steps
##' @return A tibble with the preprocessed results
preprocess_data <- function(recipe) {
    bleeding_recipe %>%
        prep() %>%
        juice()
}

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

split <- initial_split(dataset, prop = 0.75, strata = bleeding_after)
train <- training(split)
test <- testing(split)

bleeding_recipe <- recipe(bleeding_after ~ ., data = train) %>%
    update_role(index_id, new_role = "id") %>%
    update_role(nhs_number, new_role = "nhs_number") %>%
    update_role(index_date, new_role = "date") %>%
    update_role(ischaemia_after, new_role = "ischaemic_after") %>%
    step_nzv(all_predictors()) %>%
    step_normalize(all_numeric_predictors())

