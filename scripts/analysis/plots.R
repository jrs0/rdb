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


##' Plot long-format ROC curves from multiple resample (groups
##' identified by resample_id)
plot_resample_roc_curves <- function(resample_roc_curves) {
    resample_roc_curves %>%
        ggplot(aes(x = 1 - specificity,
                   y = sensitivity,
                   colour = resample_id,
                   group = resample_id)) +
        geom_line() +
        facet_wrap(~ outcome) +
        geom_abline(slope = 1, intercept = 0, size = 0.4) +
        theme_minimal(base_size = 16)        
}

##' Plot long-format lift curves from multiple resample (groups
##' identified by resample_id)
plot_resample_lift_curves <- function(resample_lift_curves) {
    resample_lift_curves %>%
        ggplot(aes(x = .percent_tested,
                   y = .lift,
                   colour = resample_id,
                   group = resample_id)) +
        geom_line() +
        facet_wrap(~ outcome) +
        theme_minimal(base_size = 16) +
        labs(x = "% Tested", y = "Lift")
}

##' Plot long-format gain curves from multiple resample (groups
##' identified by resample_id)
plot_resample_gain_curves <- function(model_results) {

    browser()
    resample_gain_curves <- model_results$gain_curves
    
    
    percentage_all_positve <- model_results$predictions %>%
        .pred_class %>%
    optimal_gain_slope <- 100/percentage_all_positive
    
    line_segments <- tibble::tribble(
                                 ~x0, ~y0, ~x1, ~y1,  ~curve,
                                 0,     0,  100,  100, "baseline",
                                 0,     0,   70,  100, "optimal",
                                 70,  100,  100,  100, "optimal"
    )
    resample_gain_curves %>%
        ggplot() +
        geom_line(aes(x = .percent_tested,
                      y = .percent_found,
                      colour = resample_id,
                      group = resample_id),
                  data = resample_gain_curves) +
        facet_wrap(~ outcome) +
        geom_segment(aes(x = x0, y = y0,
                         xend = x1, yend = y1,
                         color = curve),
                     data = line_segments) +
        theme_minimal(base_size = 16) +
        labs(x = "% Tested", y = "% Found")
}
