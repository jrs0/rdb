library(tidyverse)

##' Compute percentages grouped by outcome (whether bleeding or
##' ischaemia occurred after)
##' @param dataset The dataset to average
##' @param pred The condition to compute the proportion of
##' @param column_name The name of the column in the output tibble
##' @return A summary tibble containing the output
percentages_by_outcome <- function(dataset, pred, column_name) {
    dataset %>%
        group_by(`Bleeding After` = bleeding_after != 0, `Ischaemia After` = ischaemia_after != 0) %>%
        summarize({{ column_name }} := percent(mean({{ pred }}))) %>%
        arrange({{ column_name }})
}




##' Compute percentages by 
percentages_by_stemi <- function(dataset, pred, column_name) {
    dataset %>%
        group_by(Presentation = stemi_presentation) %>%
        summarize({{ column_name }} := percent(mean({{ pred }}))) %>%
        arrange({{ column_name }})
}


'''

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

'''
