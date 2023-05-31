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
acs_index_with_all_attributes <- raw_dataset %>%
    mutate(index_id = row_number()) %>%
    inner_join(primary_care_attribute_history,
              by = "nhs_number",
              relationship = "many-to-many")

na_means_zero <- c(
    "abortion",
    "adhd",
    "af",
    "amputations",
    "anaemia_iron",
    "anaemia_other",
    "angio_anaph",
    "arrhythmia_other",
    "asthma",
    "asylum_seeker_refugee",
    "autism",
    "back_pain",
    "cancer_bladder",
    "cancer_bladder_year",
    "cancer_bowel",
    "cancer_bowel_year",
    "cancer_breast",
    "cancer_breast_year",
    "cancer_cervical",
    "cancer_cervical_year",
    "cancer_giliver",
    "cancer_giliver_year",
    "cancer_headneck",
    "cancer_headneck_year",
    "cancer_kidney",
    "cancer_kidney_year",
    "cancer_leuklymph",
    "cancer_leuklymph_year",
    "cancer_lung",
    "cancer_lung_year",
    "cancer_melanoma",
    "cancer_melanoma_year",
    "cancer_metase",
    "cancer_metase_year",
    "cancer_other",
    "cancer_other_year",
    "cancer_ovarian",
    "cancer_ovarian_year",
    "cancer_prostate",
    "cancer_prostate_year",
    "cardio_other",
    "cataracts",
    "cc_AIDS",
    "cc_asthma",
    "cc_bronchiectasis",
    "cc_copd",
    "cc_cystic_fibrosis",
    "cc_dementia",
    "cc_diabetes_end_complications",
    "cc_diabetes_no_complications",
    "cc_heart_failure",
    "cc_hemiplegia",
    "cc_mild_liver_disease",
    "cc_moderate_or_severe_liver_disease",
    "cc_myocardial_infarction",
    "cc_peptic_ulcer",
    "cc_periph_vasc",
    "cc_pulmonary_fibrosis",
    "cc_stroke",
    "ckd",
    "coag",
    "coeliac",
    "contraception",
    "copd",
    "Covid_higher_risk_CMO",
    "Covid_increased_risk_CMO",
    "cystic_fibrosis",
    "dementia",
    "dep_alcohol",
    "dep_benzo",
    "dep_cannabis",
    "dep_cocaine",
    "dep_opioid",
    "dep_other",
    "depression",
    "diabetes_1",
    "diabetes_2",
    "diabetes_gest",
    "diabetes_retina",
    "disorder_eating",
    "disorder_pers",
    "dna_cpr",
    "down_syndrome_chrom",
    "eczema",
    "endocrine_other",
    "endometriosis",
    "eol_plan",
    "epaccs",
    "epilepsy",
    "fatigue",
    "fragility",
    "gout",
    "has_carer",
    "health_check",
    "hearing_impair",
    "hep_b",
    "hep_c",
    "hf",
    "hiv",
    "home_oxygen",
    "homeless",
    "housebound",
    "ht",
    "ibd",
    "ibs",
    "ihd_mi",
    "ihd_nonmi",
    "incont_urinary",
    "inflam_arthritic",
    "is_carer",
    "learning_diff",
    "learning_dis",
    "live_birth",
    "liver_alcohol",
    "liver_nafl",
    "liver_other",
    "lung_restrict",
    "macular_degen",
    "measles_mumps",
    "migraine",
    "miscarriage",
    "mmr1",
    "mmr2",
    "mnd",
    "ms",
    "neuro_pain",
    "neuro_various",
    "newborn_check",
    "nh_rh",
    "nose",
    "obesity",
    "organ_transplant",
    "osteoarthritis",
    "osteoporosis",
    "parkinsons",
    "pelvic",
    "phys_disability",
    "poly_ovary",
    "pre_diabetes",
    "pregnancy",
    "psoriasis",
    "ptsd",
    "qof_af",
    "qof_asthma",
    "qof_chd",
    "qof_ckd",
    "qof_copd",
    "qof_dementia",
    "qof_depression",
    "qof_diabetes",
    "qof_epilepsy",
    "qof_hf",
    "qof_ht",
    "qof_learndis",
    "qof_mental",
    "qof_obesity",
    "qof_osteoporosis",
    "qof_pad",
    "qof_pall",
    "qof_rheumarth",
    "qof_stroke",
    "sad",
    "screen_aaa",
    "screen_bowel",
    "screen_breast",
    "screen_cervical",
    "screen_eye",
    "self_harm",
    "sickle",
    "smi",
    "stomach",
    "stroke",
    "sup_metric_12",
    "sup_metric_13",
    "sup_metric_17",
    "sup_metric_41",
    "sup_metric_8",
    "tb",
    "thyroid",
    "uterine",
    "vasc_dis",
    "veteran",
    "visual_impair")

acs_dataset <- acs_index_with_all_attributes %>%
    ## Remove any rows with attributes from after the index
    ## event (the attributes are from month beginning attribute_period).
    filter(index_date > attribute_period %m+% months(1)) %>%
    ## Keep only the most recent attribute row for each index event
    group_by(index_id) %>%
    filter(attribute_period == max(attribute_period)) %>%
    ## Drop columns that are hard to process for now
    select(-bp_date, -bp_reading, -ricketts) %>%
    names() %>%
    setdiff(., na_means_zero)


acs_dataset

    setdiff()
    
    mutate_at(na_means_zero, ~replace_na(.,0))

