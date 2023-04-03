raw_dataset <- readRDS("gendata/medium_dataset.rds")

dataset <- raw_dataset %>%
    mutate(bleeding_after = factor(bleeding_after == 0, labels = c("bleed_occurred", "no_bleed"))) %>%
    mutate(ischaemia_after = factor(ischaemia_after == 0, labels = c("ischaemia_occurred", "no_ischaemia"))) %>%
    drop_na() %>%
    ## Remove the after columns
    dplyr::select(- (matches("after") & !matches("(bleeding_after|ischaemia_after)")), - nhs_number) %>%
    ## Add an ID to link up patients between bleeding and ischaemia predictions
    mutate(patient_id = as.factor(row_number()))

tree_mod <- decision_tree() %>%
    set_engine("rpart") %>%
    set_mode("classification")

fit <- tree_mod %>%
    fit(ischaemia_after ~ age_at_index,  data = dataset)
