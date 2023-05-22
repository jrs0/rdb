##' @title Read the code groups defined in the supplied file. 
##' @param file The code group file to read 
##' @return A named list mapping group names to a tibble of codes
##' @export
code_groups <- function(file) {
    result <- dump_groups(file)
    purrr::map(result, ~ tibble::as_tibble(.x))
}

##' Returns a flat tibble of duplicated codes and descriptions,
##' one row per instance of the code in a group, which a column
##' "duplicate_count" indicating how many times a code has been
##' duplicated. The returned tibble is arranged by the code
##' column to see duplicated groups together.
##'
##' @title Return a tibble of duplicated codes in code groups
##' @param code_groups A named list of code groups (named
##' by the name of the group
##' @return A tibble of duplicated codes 
##'
find_code_duplicates <- function(code_groups) {    
    list(names(code_groups), code_groups) %>%
        purrr::pmap(~ .y %>% dplyr::mutate(group = .x)) %>%
        purrr::list_rbind() %>%
        dplyr::group_by(names) %>%
        dplyr::mutate(duplicate_count = dplyr::n()) %>%
        dplyr::filter(duplicate_count > 1) %>%
        dplyr::ungroup() %>%
        dplyr::arrange(names)
}
