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



roc_curves %>%
    filter(outcome_name == "ischaemia") %>%
    left_join(roc_summary, by = join_by(model_name, outcome_name))%>%
    mutate(model_name = str_to_title(str_replace(model_name, "_", " "))) %>%
    ggplot(aes(x = 1 - specificity, y = sensitivity,
               color = primary,
               group = interaction(model_name, outcome_name, model_id))) +
    geom_line(data = . %>% filter(primary == "bootstrap"),aes(col = "Bootstrap"), alpha = 0.5, size =1.05) +
    geom_line(data = . %>% filter(primary == "primary"),aes(col = "Primary"),size = 1.05) +
    geom_abline(slope = 1, intercept = 0, size = 0.4) +
    geom_text(aes(x = 0.75, y = 0.25, label = glue::glue("AUC = {round(`Mean AUC`[.data$primary == primary], digits = 2)}")),
              col = "black",  hjust =0, vjust = 0) +
    coord_fixed() +
    theme_minimal(base_size = 16) +
    labs(title = "ROC curves for the ischaemia risk models") +
    facet_wrap( ~ model_name, ncol=2)
