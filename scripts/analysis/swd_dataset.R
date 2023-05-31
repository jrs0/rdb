library(tidyverse)
library(icdb)
library(lubridate)

## Load rdb package here. Set the working directory to
## the location of this script

use_cache(TRUE, lifetime = ddays(30))
msrv <- mapped_server("xsw")

## Load the raw data from file or database
raw_dataset <- processed_acs_dataset("../../scripts/config.yaml")
raw_dataset

## Doing this as a right_join to copy data to the remote sql as a temporary
## table. Doing this is faster than just fetching all the data (30 seconds
## vs 10 minutes). The inner join excludes nhs_numbers which have no row at
## all in the swd (in order to interpret subsequent NA as lack of condition
## rather than missingness)
nhs_numbers <- raw_dataset %>%
    distinct(nhs_number)
primary_care_attribute_history <- msrv$swd$swd_attribute_history %>%
    inner_join(nhs_numbers, by = "nhs_number", copy = TRUE) %>%
    run()

## Join the patient attributes to the acs index events, reducing to only
## the nhs_numbers with a row in the primary care attributes.
raw_dataset %>%
    mutate(index_id = row_number()) %>%
    inner_join(primary_care_attribute_history,
              by = "nhs_number",
              relationship = "many-to-many") %>%
    count(index_id) %>%
    arrange(desc(n))
