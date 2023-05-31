##' Converts the outcome columns to two level factors and
##' removes mortality columns (mortality is still included
##' in the ischaemia outcome)
modelling_dataset <- function(raw_dataset) {
    raw_dataset %>%
        count_to_two_level_factor(bleeding_after) %>%
        count_to_two_level_factor(ischaemia_after) %>%
        remove_other_outcome_columns() %>%
        remove_mortality_columns() %>%
        drop_na() %>%
        mutate(index_id = as.factor(row_number()))
}
