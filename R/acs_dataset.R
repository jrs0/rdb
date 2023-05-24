##' @importFrom rlang .data
NULL 

##' Loading the full dataset from the database takes a long time
##' (about 20 minutes), so this function is a wrapper around
##' make_acs_dataset() which saves the result to a file and reads
##' from the file if specified in the config file.
##'
##' If the dataset is loaded from the database, then it is stored
##' in the location specified in the file block of the config file.
##'
##' @title Load the ACS dataset from the database or a file
##' @param config_path The path to the YAML configuration file
##' @return A tibble of the dataset
##' 
load_acs_dataset <- function(config_path = "config.yaml") {
    config <- yaml::read_yaml(config_path)
    directory <- config$file$directory
    file_name <- config$file$file_name
    if (is.null(directory) || is.null(file_name)) {
        stop("Missing required directory or file_name in config")
    } 
    file_path <- fs::path(directory, file_name)
    
    if (config$load_from_file) {
        if (!fs::dir_exists(directory)) {
            stop("Directory ", directory, " does not exist. Set load_from_file: false.")
        } else if (!fs::file_exists(file_path)) {
            stop("Dataset file ", file_path, " does not exist. Set load_from_file: false.")
        }
        readRDS(file_path)
    } else {
        dataset <- tibble::as_tibble(make_acs_dataset(config_path))
        if (!fs::dir_exists(directory)) {
            message("Creating missing ", directory, " for storing dataset")
            fs::dir_create(directory)
        }
        saveRDS(dataset, file_path)
        dataset
    }
}
##' This functions creates an ischaemia_after column by adding up the various
##' ischaemia-related columns. In addition, the unix timestamp columns are
##' converted to lubridate
##'
##' @title Convert the acs dataset to the form needed by the analysis scripts
##' @param config_file The path to the config file for the dataset
##' @return A tibble of the acs dataset
processed_acs_dataset <- function(config_file = "config.yaml") {

    dataset <- load_acs_dataset(config_file)

    ## Convert the date from unix timestamps to lubridate
    dataset <- dplyr::mutate(dataset,
                             index_date = lubridate::as_datetime(.data$index_date),
                             survival_time = lubridate::as.duration(.data$survival_time))

    ## Compute the outcome columns (only need ischaemia because bleed
    ## is already in there). The ischaemia endpoint is defined as
    ## subsequent ACS STEMI or NSTEMI, ischaemic stroke, or cardiac death.
    dataset <- dplyr::mutate(dataset,
                             ischaemia_after = .data$acs_stemi_after +
                                 .data$acs_nstemi_after +
                                 .data$all_ischaemia_after +
                                 .data$ischaemic_stroke_after +
                                 (.data$cause_of_death == "cardiac"))
}
