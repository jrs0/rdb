## Go careful trying to debug a c++ function through an R call --
## load_all does not know how to reload R functions properly that
## depend on a c++ function, so you can be changing the c++ and
## wondering why nothing is happening -- either call the c++ function
## directly to debug, or make a change to this file to force it to
## reload.

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
                             index_date = lubridate::as_datetime(index_date),
                             index_to_death = lubridate::as.duration(index_to_death))
    
    ## Compute the outcome columns (only need ischaemia because bleed
    ## is already in there)
    dataset <- dplyr::mutate(dataset,
                             ischaemia_after = acs_stemi_after
                             + acs_nstemi_after
                             + ischaemic_stroke_after)
}
