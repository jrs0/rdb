library(tidymodels)

##' Requires exactly one NZV (near-zero variance) step
##' @param recipe The recipe containing the step_nzv()
##' @return A character vector of predictor column names
##' that are removed due to NZV
near_zero_variance_columns <- function(recipe) {
    nzv_row_numbers <- recipe %>%
        prep() %>%
        tidy() %>%
        filter(type == "nzv")

    if (nrow(nzv_row_numbers) != 1) {
        stop("There must be exactly one NZV predictor in the recipe")
    }
    
    recipe %>%
        prep() %>%
        tidy(number = nzv_row_numbers[[1]]) %>%
        pull(terms)
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

##' This function performs the step_nzv(), but not any of the
##' others, so that you can see the original data from the columns
##' that will be included in the model.
##' 
##' @param recipe The recipe defining the preprocessing steps
##' @return The dataset without the near-zero variance columns
##' 
without_near_zero_variance_columns <- function(recipe) {
    nzv_cols <- near_zero_variance_columns(recipe)
    retained_cols <- bleeding_recipe %>%
        summary() %>%
        filter(!(variable %in% nzv_cols)) %>%
        pull(variable)
    ## template contains the data used to create the recipe?
    recipe$template %>%
        select(retained_cols)
}

percentages_by_outcome <- function(dataset, pred, column_name) {
    dataset %>%
        group_by(`Bleeding After` = bleeding_after != 0, `Ischaemia After` = ischaemia_after != 0) %>%
        summarize({{ column_name }} := percent(mean({{ pred }}))) %>%
        arrange({{ column_name }})
}

percentages_by_stemi <- function(dataset, pred, column_name) {
    dataset %>%
        group_by(Presentation = stemi_presentation) %>%
        summarize({{ column_name }} := percent(mean({{ pred }}))) %>%
        arrange({{ column_name }})
}


presented_stemi_by_outcome <- raw_dataset %>%
    percentages_by_outcome(stemi_presentation == "STEMI",
                         `Presented STEMI`)

had_pci_by_outcome <- raw_dataset %>%
    percentages_by_outcome(index_type == "PCI",
                         `Had PCI`)

any_cause_death_by_outcome <- raw_dataset %>%
    percentages_by_outcome(cause_of_death != "no_death",
                         `Any Cause Death`)

cardiovascular_death_by_outcome <- raw_dataset %>%
    percentages_by_outcome(cause_of_death == "cardiac",
                         `Cardiovascular Death`)

cardiovascular_death_by_presentation <- raw_dataset %>%
    percentages_by_stemi(cause_of_death == "cardiac",
                         `Cardiovascular Death`)

any_cause_death_by_presentation <- raw_dataset %>%
    percentages_by_stemi(cause_of_death != "no_death",
                         `Any Cause Death`)



raw_dataset <- processed_acs_dataset("../../config.yaml")

## Simple proportions from the data
    
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

bleeding_recipe %>%
    near_zero_variance_predictors()

after_preprocessing <- bleeding_recipe %>%
    preprocess_data()

after_nzv_removal <- bleeding_recipe %>%
    without_near_zero_variance_columns()

log_reg <- logistic_reg() %>% 
    set_engine('glm') %>% 
    set_mode('classification')

workflow <- workflow() %>%
    add_model(log_reg) %>%
    add_recipe(bleeding_recipe)

workflow %>%
    fit(data = train)
