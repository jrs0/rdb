library(tidyverse)
library(ggplot2)

##' @title Plot the distribution of index events with time
##'
##' The plot breaks down the results by stemi/nstemi presentation.
##'
##' @param dataset The dataset produced with acs_dataset()
##' @return A ggplot object
##' 
plot_index_with_time <- function(dataset) {
    dataset %>%
        rename(STEMI = stemi_presentation) %>%
        ggplot() +
        geom_histogram(aes(x = index_date, fill = STEMI),
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
        mutate(stemi_presentation = factor(stemi_presentation, labels = c("NSTEMI", "STEMI"))) %>%
        select(stemi_presentation, cause_of_death, index_to_death) %>%
        ggplot(aes(x = index_to_death, fill = cause_of_death)) +
        geom_density(alpha = 0.3, position="stack") +
        facet_grid(~ stemi_presentation) +
        labs(x = "Survival time (seconds)", y = "Distribution (normalised by totals)") +
        theme_minimal(base_size = 16)
}
