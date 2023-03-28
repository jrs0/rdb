##' .. content for \description{} (no empty lines) ..
##'
##' @title Create the ACS/PCI bleeding/ischaemia risk dataset
##' @param config_file 
##' @return A tibble of the dataset
##' @export 
acs_dataset <- function(config_file = "config.yaml") {

    ## Get the raw data
    dataset <- tibble::as_tibble(make_acs_dataset(config_file))

    ## Convert the date from unix time to lubridate
    dataset <- dplyr::mutate(dataset,
                             index_date = lubridate::as_datetime(index_date))
    
}
