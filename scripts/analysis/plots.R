library(tidyverse)
library(ggplot2)
library(corrr)

##' @title Plot the distribution of index events with time
##'
##' The plot breaks down the results by stemi/nstemi presentation.
##'
##' @param dataset The dataset produced with acs_dataset()
##' @return A ggplot object
##' 
plot_index_with_time <- function(dataset) {
    dataset %>%
        rename(Presentation = stemi_presentation) %>%
        ggplot() +
        geom_histogram(aes(x = index_date, fill = Presentation),
                       bins = 75)
}

##' Plot the distribution of predictor counts with the four possible outcomes
##' of bleeding/ischaemia. These are the counts of events before the index
##' event
##'
##' @param dataset The dataset produced with acs_dataset()
##' @return A ggplot object
##' 
plot_predictors_distributions <- function(dataset) {

    ## Collapse the columns into broader groups
    reduced <- dataset %>%
        ## Keep the predictors
        mutate(`STEMI ACS` = acs_stemi_before,
               `NSTEMI ACS` = acs_nstemi_before + acs_unstable_angina_before,
               `Renal` = rowSums(across(matches("ckd.*_before"))),
               `Anaemia` = anaemia_before,
               `Atrial Fib.` = atrial_fib_after,
               `Bleeding` = bleeding_before,
               `Diabetes` = rowSums(across(matches("diabetes.*_before"))),
               `Blood Transfusion` = blood_transfusion_before,
               `CAGB` = cabg_before,
               `Cancer` = cancer_before,
               `COPD` = copd_before,
               `PCI` = pci_before,
               `Smoking` = smoking_before,
               `Liver disease` = portal_hypertension_before + cirrhosis_before,
               `Low Platelets` = thrombocytopenia_before) %>%
        ## Keep the outcomes
        mutate(`Bleeding Outcome` = factor(bleeding_after > 0,
                                           labels = c("No Bleed",
                                                      "Bleed After")),
               `Ischaemia Outcome` = factor(ischaemia_after > 0,
                                            labels = c("No Ischaemia",
                                                       "Ischaemia After"))) %>%
        dplyr::select(- matches("(before|after)"))
    
    reduced %>%
        ## Use capital letter to identify predictor
        pivot_longer(matches("[A-Z]", ignore.case = FALSE) & !matches("Outcome")) %>%
        group_by(`Bleeding Outcome`, `Ischaemia Outcome`, name) %>%
        summarise(value = mean(value)) %>%
        ggplot(aes(name, value)) +
        geom_col(fill = 'deepskyblue4') +
        facet_grid(`Bleeding Outcome` ~ `Ischaemia Outcome`) +
        labs(x = 'Predictor class', y = 'Average count') +
        theme_minimal(base_size = 16) +
        theme(axis.text.x = element_text(angle = 90, vjust = 0.5, hjust=1))    
}

##' Plot the distribution of all outcomes with the four possible outcomes
##' of bleeding/ischaemia. These are the counts of events happening after
##' the index event.
##'
##' @param dataset The dataset produced with acs_dataset()
##' @return A ggplot object
##' 
plot_outcome_distributions <- function(dataset) {

    ## Collapse the columns into broader groups
    reduced <- dataset %>%
        ## Keep the predictors
        mutate(`STEMI ACS` = acs_stemi_after,
               `NSTEMI ACS` = acs_nstemi_after + acs_unstable_angina_after,
               `Renal` = rowSums(across(matches("ckd.*_after"))),
               `Anaemia` = anaemia_after,
               `Atrial Fib.` = atrial_fib_after,
               `Diabetes` = rowSums(across(matches("diabetes.*_after"))),
               `Blood Transfusion` = blood_transfusion_after,
               `CAGB` = cabg_after,
               `Cancer` = cancer_after,
               `COPD` = copd_after,
               `PCI` = pci_after,
               `Smoking` = smoking_after,
               `Liver disease` = portal_hypertension_after + cirrhosis_after,
               `Low Platelets` = thrombocytopenia_after) %>%
        ## Keep the outcomes
        mutate(`Bleeding Outcome` = factor(bleeding_after > 0,
                                           labels = c("No Bleed",
                                                      "Bleed After")),
               `Ischaemia Outcome` = factor(ischaemia_after > 0,
                                            labels = c("No Ischaemia",
                                                       "Ischaemia After"))) %>%
        dplyr::select(- matches("(before|after)"))
    
    reduced %>%
        ## Use capital letter to identify predictor
        pivot_longer(matches("[A-Z]", ignore.case = FALSE) & !matches("Outcome")) %>%
        group_by(`Bleeding Outcome`, `Ischaemia Outcome`, name) %>%
        summarise(value = mean(value)) %>%
        ggplot(aes(name, value)) +
        geom_col(fill = 'deepskyblue4') +
        facet_grid(`Bleeding Outcome` ~ `Ischaemia Outcome`) +
        labs(x = 'Predictor class', y = 'Average count') +
        theme_minimal(base_size = 16) +
        theme(axis.text.x = element_text(angle = 90, vjust = 0.5, hjust=1))    
}


##' Distribution of non-zero bleeding/ischaemia counts (uses a cutoff)
plot_non_zero_count_distributions <- function(dataset) {
    total_rows <- nrow(dataset)
    dataset %>%
        pivot_longer(c(bleeding_after, ischaemia_after)) %>%
        filter(value != 0 & value < 20) %>%
        ggplot(aes(x = value)) +
        geom_histogram(binwidth = 1, fill = 'deepskyblue4') +
        facet_grid(~ name) +
        labs(title = paste("Distribution of non-zero counts (from", total_rows, "rows)"),
             x = "Total count",
             y = "Total rows having count") +
        theme_minimal(base_size = 16)
}

##' Plot the distribution of age in each of the four bleeding/ischaemia
##' groups
##'
##' @param dataset The dataset produced with acs_dataset()
##' @return A ggplot object
##' 
plot_age_distributions <- function(dataset) {
    ## Collapse the columns into broader groups
    reduced <- dataset %>%
        ## Keep the predictors
        mutate(Age = age_at_index) %>%
        ## Keep the outcomes
        mutate(`Bleeding Outcome` = factor(bleeding_after > 0,
                                           labels = c("No Bleed",
                                                      "Bleed After")),
               `Ischaemia Outcome` = factor(ischaemia_after > 0,
                                            labels = c("No Ischaemia",
                                                       "Ischaemia After"))) %>%
        dplyr::select(- matches("(before|after)"))

    
    reduced %>%
        drop_na() %>%
        ## Use capital letter to identify predictor
        group_by(`Bleeding Outcome`, `Ischaemia Outcome`) %>%
        ggplot(aes(x = Age)) +
        geom_histogram(fill = 'deepskyblue4') +
        facet_grid(`Bleeding Outcome` ~ `Ischaemia Outcome`) +
        labs(x = 'Predictor class', y = 'Count in bin') +
        theme_minimal(base_size = 16) +
        theme(axis.text.x = element_text(angle = 90, vjust = 0.5, hjust=1))    
}

##' Plot the distribution of survival time from index for the
##' STEMI and NSTEMI groups
##' 
##' @title Plot distribution of survival times from index 
##' @param dataset ACS dataset
##' @return A ggplot object
##' 
plot_survival <- function(dataset) {
    dataset %>%
        filter(cause_of_death != "no_death") %>%
        select(stemi_presentation, cause_of_death, survival_time) %>%
        mutate(survival_time = survival_time / (24*60*60)) %>%
        rename(`Cause of Death` = cause_of_death) %>%
        ggplot(aes(x = survival_time, fill = `Cause of Death`)) +
        geom_density(alpha = 0.3, position="stack") +
        facet_grid(~ stemi_presentation) +
        labs(x = "Survival time (days)", y = "Distribution (normalised by totals)") +
        theme_minimal(base_size = 16)
}

plot_predictor_outcome_correlations <- function(after_nzv_removal) {
    after_nzv_removal %>%
        select(-nhs_number, -index_date) %>%
        two_level_factor_to_numeric(bleeding_after, "bleeding_occurred") %>%
        two_level_factor_to_numeric(ischaemia_after, "ischaemia_occurred") %>%
        two_level_factor_to_numeric(index_type, "PCI") %>%
        two_level_factor_to_numeric(stemi_presentation, "STEMI") %>%
        select(bleeding_after, ischaemia_after, everything()) %>%
        correlate() %>%
        rplot(colors = c("blue", "red")) +
        theme(axis.text.x = element_text(angle = 90, vjust = 0.5, hjust=1))
}

optimal_gain_line_segments <- function(model_results) {
    percent_positive <- model_results$predictions %>%
        mutate(truth = if_else(
                   outcome == "bleeding",
                   bleeding_after,
                   ischaemia_after
               )) %>%
        group_by(outcome) %>%
        summarize(mean = 100*sum(truth == "occurred") / n())


    line_segments <- percent_positive %>%
        pmap(function(outcome, mean) {
            tibble::tribble(
                        ~x0, ~y0, ~x1, ~y1,  ~curve, ~outcome,
                        0,     0,  100,  100, "baseline", outcome, 
                        0,     0,   mean,  100, "optimal", outcome,
                        mean,  100,  100,  100, "optimal", outcome
                    )            
        }) %>%
        bind_rows() %>%
        unique()    
}

##' Plot long-format gain curves from multiple resample (groups
##' identified by resample_id)
plot_resample_gain_curves <- function(model_results) {

    line_segments <- model_results %>%
        optimal_gain_line_segments()

    gain_curves <- model_results$gain_curves
    ggplot() +
        geom_line(aes(x = .percent_tested,
                      y = .percent_found,
                      colour = resample_id,
                      group = resample_id),
                  data = gain_curves) +
        facet_wrap(~ outcome) +
        geom_segment(aes(x = x0, y = y0,
                         xend = x1, yend = y1,
                         colour = curve),
                     data = line_segments) +
        theme_minimal(base_size = 16) +
        labs(x = "% Tested", y = "% Found")
}


##' Plot long-format ROC curves from multiple resample (groups
##' identified by resample_id)
plot_resample_roc_curves <- function(model_results) {

    line_segments <- tibble::tribble(
                ~x0, ~y0, ~x1, ~y1,  ~curve,
                0,     0,  1,  1, "baseline",
                0,     0,   0,  1, "optimal",
                0,  1,  1,  1, "optimal"
            )            
    
    roc_curves <- model_results$roc_curves
    roc_aucs <- model_results$model_aucs %>%
        group_by(outcome) %>%
        summarize(mean_auc = mean(.estimate) %>%
                      round(digits = 2))
    ggplot() +
        geom_line(aes(x = 1 - specificity,
                      y = sensitivity,
                      colour = resample_id,
                      group = resample_id),
                  data = roc_curves) +
        geom_segment(aes(x = x0, y = y0,
                         xend = x1, yend = y1,
                         colour = curve),
                     data = line_segments) +
        geom_text(aes(x = 0.75, y = 0.25,
                      label = paste("AUC =", mean_auc)),
                  data = roc_aucs) +
        facet_wrap(~ outcome) +
        theme_minimal(base_size = 16)        
}



##' Plot long-format lift curves from multiple resample (groups
##' identified by resample_id)
plot_resample_lift_curves <- function(model_results) {
    model_results$lift_curves %>%
        ggplot(aes(x = .percent_tested,
                   y = .lift,
                   colour = resample_id,
                   group = resample_id)) +
        geom_line() +
        facet_wrap(~ outcome) +
        theme_minimal(base_size = 16) +
        labs(x = "% Tested", y = "Lift")
}

##' Plot long-format lift curves from multiple resample (groups
##' identified by resample_id)
plot_resample_precision_recall_curves <- function(model_results) {
    model_results$precision_recall_curves %>%
        ggplot(aes(x = precision,
                   y = recall,
                   colour = resample_id,
                   group = resample_id)) +
        geom_line() +
        facet_wrap(~ outcome) +
        theme_minimal(base_size = 16) +
        labs(x = "Recall (True Positive Rate)",
             y = "Precision (Positive Predictive Value)")
}

##' For one patient, plot all the model predictions. This graph is supposed
##' to test how well the models agree with one another. The bootstrap models
##' and the primary model are all plotted as dots (not distinguished).
##'
##' TODO: fix this function
plot_variability_in_risk_predictions <- function(all_model_results) {
    grouped_pred %>%
        ungroup() %>%
        dplyr::select(index_id, model_name, outcome_name,
                      primary, model_id, .pred_occurred) %>%
        pivot_wider(names_from = "outcome_name",
                    values_from = ".pred_occurred") %>%
        ## Pick only one patient. The idea is to plot all the predictions for just
        ## that one patient
        filter(index_id %in% sample(index_id, size = 4)) %>%
        ## Uncomment to view all models for some patients
        ##filter(id %in% c(1,20,23,43, 100, 101, 102)) %>%
        ggplot(aes(x = bleed, y = ischaemia, color=model_name, shape = primary)) +
        geom_point() +
        ## scale_y_log10() +
        ## scale_x_log10() +
        labs(title = "Plot of risk predictions from all models for some patients") +
        facet_wrap( ~ index_id, ncol=2) +
        theme_minimal(base_size = 16)

}

##' Plot of the risk trade-off graph for each model (using the primary model)
##'
##' TODO: fix this function
plot_risk_tradeoffs <- function(model_results) {
    grouped_pred %>%
        ungroup() %>%
        filter(primary == "primary") %>%
        dplyr::select(stemi_presentation, index_id, outcome_name, model_name, .pred_occurred) %>%
        pivot_wider(names_from = outcome_name, values_from = .pred_occurred) %>%
        ggplot(aes(x = bleed, y = ischaemia, color = stemi_presentation)) +
        geom_point() +
        facet_wrap( ~ model_name, ncol=2) +
        theme_minimal(base_size = 16)
}
