library(tidyverse)
library(icdb)
library(lubridate)

## Load rdb package here. Set the working directory to
## the location of this script

use_cache(TRUE, lifetime = ddays(30))
msrv <- mapped_server("xsw")

## Load the raw data from file or database
raw_dataset <- processed_acs_dataset("../../scripts/config.yaml")


primary_care_attribute_history <- msrv$swd$swd_attribute_history %>%
    run()
