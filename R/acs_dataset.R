##' .. content for \description{} (no empty lines) ..
##'
##' @title Create the ACS/PCI bleeding/ischaemia risk dataset
##' @param config_file 
##' @return A tibble of the dataset
##' @export 
acs_dataset <- function(config_file = "config.yaml") {
    dataset <- tibble::as_tibble(make_acs_dataset(config_file))
    dataset <- dplyr::mutate(dataset,
                             index_date = lubridate::as_datetime(index_date))
    
}
