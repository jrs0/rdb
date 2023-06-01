library(tidyverse)

devtools::load_all("../../")

source("dataset.R")
source("descriptive.R")

## Load the raw data from file or database (HES only)
##raw_dataset <- load_hes_dataset("../config.yaml")

## Load the raw data from file or database (HES + SWD)
raw_dataset <- load_swd_dataset("../config.yaml")

num_index_events <- raw_dataset %>%
    nrow()

first_index_date <- raw_dataset %>%
    pull(index_date) %>%
    min()

last_index_date <- raw_dataset %>%
    pull(index_date) %>%
    max()

num_patients <- raw_dataset %>%
    distinct(nhs_number) %>%
    nrow()

percentage_events_after_index <- raw_dataset %>% 
    any_cause_death_by_stemi() %>%
    left_join(cardiovascular_death_by_stemi(raw_dataset),
              by = "Presentation") %>%
    left_join(ischaemia_by_stemi(raw_dataset),
              by = "Presentation") %>%   
    left_join(bleeding_by_stemi(raw_dataset),
              by = "Presentation")    
    
