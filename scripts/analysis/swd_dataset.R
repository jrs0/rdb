library(tidyverse)
library(icdb)
library(lubridate)

## Load rdb package here. Set the working directory to
## the location of this script
setwd("scripts/analysis")

source("dataset.R")
source("model.R")
source("plots.R")
source("descriptive.R")

use_cache(TRUE, lifetime = ddays(30))
msrv <- mapped_server("xsw")

## Load the raw data from file or database
raw_dataset <- processed_acs_dataset("../../scripts/config.yaml")

dataset <- raw_dataset %>%
    modelling_dataset()

## Doing this as a right_join to copy data to the remote sql as a temporary
## table. Doing this is faster than just fetching all the data (30 seconds
## vs 10 minutes). The inner join excludes nhs_numbers which have no row at
## all in the swd (in order to interpret subsequent NA as lack of condition
## rather than missingness)
nhs_numbers <- dataset %>%
    distinct(nhs_number)
primary_care_attribute_history <- msrv$swd$swd_attribute_history %>%
    inner_join(nhs_numbers, by = "nhs_number", copy = TRUE) %>%
    run()

## Join the patient attributes to the acs index events, reducing to only
## the nhs_numbers with a row in the primary care attributes.
acs_index_with_all_attributes <- dataset %>%
    inner_join(primary_care_attribute_history,
              by = "nhs_number",
              relationship = "many-to-many")

## These are the columns where NA means zero. There are inevitably going
## to be some cases where NA means missing -- can only be fixed in the
## database
na_means_zero <- c(
    "abortion","adhd","af","amputations","anaemia_iron",
    "anaemia_other","angio_anaph","arrhythmia_other","asthma","autism",
    "back_pain","cancer_bladder","cancer_bladder_year",
    "cancer_bowel","cancer_bowel_year","cancer_breast",
    "cancer_breast_year","cancer_cervical","cancer_cervical_year","cancer_giliver",
    "cancer_giliver_year","cancer_headneck","cancer_headneck_year",
    "cancer_kidney","cancer_kidney_year","cancer_leuklymph","cancer_leuklymph_year",
    "cancer_lung","cancer_lung_year","cancer_melanoma","cancer_melanoma_year",
    "cancer_metase","cancer_metase_year","cancer_other","cancer_other_year",
    "cancer_ovarian","cancer_ovarian_year","cancer_prostate",
    "cancer_prostate_year","cardio_other","cataracts","ckd","coag","coeliac",
    "contraception","copd","cystic_fibrosis","dementia","dep_alcohol","dep_benzo",
    "dep_cannabis","dep_cocaine","dep_opioid","dep_other","depression",
    "diabetes_1","diabetes_2","diabetes_gest","diabetes_retina","disorder_eating",
    "disorder_pers","dna_cpr","eczema","endocrine_other","endometriosis",
    "eol_plan","epaccs","epilepsy","fatigue","fragility",
    "gout","has_carer","health_check","hearing_impair","hep_b",
    "hep_c","hf","hiv","homeless","housebound",
    "ht","ibd","ibs","ihd_mi","ihd_nonmi",
    "incont_urinary","inflam_arthritic","is_carer","learning_diff","learning_dis",
    "live_birth","liver_alcohol","liver_nafl","liver_other","lung_restrict",
    "macular_degen","measles_mumps","migraine","miscarriage","mmr1",
    "mmr2","mnd","ms","neuro_pain","neuro_various",
    "newborn_check","nh_rh","nose","obesity","organ_transplant",
    "osteoarthritis","osteoporosis","parkinsons","pelvic","phys_disability",
    "poly_ovary","pre_diabetes","pregnancy","psoriasis","ptsd",
    "qof_af","qof_asthma","qof_chd","qof_ckd","qof_copd",
    "qof_dementia","qof_depression","qof_diabetes","qof_epilepsy","qof_hf",
    "qof_ht","qof_learndis","qof_mental","qof_obesity","qof_osteoporosis",
    "qof_pad","qof_pall","qof_rheumarth","qof_stroke","sad",
    "screen_aaa","screen_bowel","screen_breast","screen_cervical","screen_eye",
    "self_harm","sickle","smi","stomach","stroke",
    "tb","thyroid","uterine","vasc_dis","veteran","visual_impair")

acs_dataset <- acs_index_with_all_attributes %>%
    ## Remove any rows with attributes from after the index
    ## event (the attributes are from month beginning attribute_period).
    filter(index_date > attribute_period %m+% months(1)) %>%
    ## Keep only the most recent attribute row for each index event
    group_by(index_id) %>%
    filter(attribute_period == max(attribute_period)) %>%
    ungroup() %>%
    ## Remove the attribute period column
    select(-attribute_period) %>%
    ## Drop columns that are hard to process for now (todo: move
    ## to mapping file)
    select(-bp_date, -bp_reading, -ricketts, -qof_cancer, -cancer_nonmaligskin, -cancer_nonmaligskin_year) %>%
    ## Certain columns use Null to indicate zero. Replace these with
    ## zero.
    mutate_at(na_means_zero, ~ replace_na(.,0))
    
## Find columns with high NA proportion
acs_dataset %>%
    summarise_all(~ mean(is.na(.x))) %>%
    pivot_longer(cols = everything()) %>%
    arrange(desc(value))

## Configure global options
training_proportion <- 0.75

## Make test/train split
split <- acs_dataset %>%
    initial_split(prop = training_proportion, strata = bleeding_after)
train <- training(split)
test <- testing(split)

bleeding_recipe <- recipe(train) %>%
    update_role(everything(), new_role = "predictor") %>%        
    update_role(bleeding_after, new_role = "outcome") %>%
    update_role(ischaemia_after, new_role = "ignored_outcome") %>%
    update_role(nhs_number, new_role = "nhs_number") %>%
    update_role(index_date, new_role = "index_date") %>%
    update_role(index_id, new_role = "index_id") %>%
    step_dummy(all_nominal_predictors()) %>%
    step_nzv(all_predictors()) %>%
    step_normalize(all_numeric_predictors())
