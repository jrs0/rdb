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
    ggplot(dataset) +
        geom_histogram(aes(x = index_date, fill = stemi_presentation),
                       bins = 75)
}

##' Plot the distribution of predictor counts with the four possible outcomes
##' of bleeding/ischaemia
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
