library(tidyverse)
library(scales)

##' Group by a set of custom named conditions and compute an average based
##' on a custom predicate
##' @param dataset The dataset to average
##' @param column_name The name of the output average column
##' @param pred_outcome The custom predicate defining the average
##' @param ... A list of named group-by conditions
##' @return A tibble with the percentages defined by the predicate broken
##' down by the custom groups
##' 
percents_by_groups <- function(dataset, column_name, pred_outcome, ...) {
    dataset %>%
        group_by(...) %>%
        summarize({{ column_name }} := percent(mean({{ pred_outcome }}))) %>%
        arrange({{ column_name }})        
}

##' Break down column average by bleeding/ischaemia outcomes
percents_by_outcomes <- function(dataset, pred_outcome, column_name) {
    dataset %>%
        percents_by_groups({{ column_name }}, {{ pred_outcome }},
                           `Bleeding After` = bleeding_after != 0,
                           `Ischaemia After` = ischaemia_after != 0)
}

##' Break down a column average by STEMI presentation
percents_by_stemi <- function(dataset, pred_outcome, column_name) {
    dataset %>%
        percents_by_groups({{ column_name }}, {{ pred_outcome }},
                           `Presentation` = stemi_presentation)
}

##' Break down a column average by whether or not the index event
##' involved a PCI
percents_by_had_pci <- function(dataset, pred_outcome, column_name) {
    dataset %>%
        percents_by_groups({{ column_name }}, {{ pred_outcome }},
                           `Had PCI` = index_type == "PCI")
}

##' Percentage of any-cause death (all-cause and cardiac) following
##' the index event
any_cause_death_by_stemi <- function(dataset) {
    dataset %>%
        percents_by_stemi(cause_of_death != "no_death",
                          "Any Cause Death")
}

##' Percentage of cardiovascular death following the index event
cardiovascular_death_by_stemi <- function(dataset) {
    dataset %>%
        percents_by_stemi(cause_of_death == "cardiac",
                          "Cardiovascular Death")
}

##'  Subsequent ischaemia by presentation
ischaemia_by_stemi <- function(dataset) {
    dataset %>%
        percents_by_stemi(ischaemia_after != 0,
                          "Ischaemia After")
}

##' Subsequent ischaemia by presentation
bleeding_by_stemi <- function(dataset) {
    dataset %>%
        percents_by_stemi(bleeding_after != 0,
                          "Bleeding After")
}


model_aucs <- function(model_results, model_name) {
    model_results$model_aucs %>%
        mutate(model = model_name)
}

summarise_model_aucs <- function(all_model_aucs) {
    all_model_aucs %>%
        filter(.metric == "roc_auc") %>%
        rename(auc = .estimate) %>%
        mutate(outcome = str_to_title(outcome)) %>%
        group_by(outcome, model) %>%
        summarise("Mean AUC" = mean(auc), "AUC Error" = 1.96*sd(auc)) %>%
        pivot_wider(names_from = outcome,
                    values_from = c(`Mean AUC`, `AUC Error`),
                    names_vary = "slowest",
                    names_glue = "{.value} ({outcome})")
}

##' TODO fix this function
get_correlations <- function(raw_dataset) {
    for_correlations <- raw_dataset %>%
        dplyr::select(bleeding_after,
                      ischaemia_after,
                      all_of(remaining_predictors))
    correlations = cor(for_correlations)
    corrplot(correlations)
    remaining_predictors
}
