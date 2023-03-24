
##' @title Read the code groups defined in the supplied file. 
##' @param file The code group file to read 
##' @return A named list mapping group names to a tibble of codes
##' @export
code_groups <- function(file) {
    result <- dump_groups(file)
    purrr::map(result, ~ tibble::as_tibble(.x))
}
