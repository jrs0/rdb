library(tidymodels)
library(tidyverse)


'''

##' Using the bootstrap resamples to obtain multiple models
##' and thereby obtain a spectrum of predicted class scores for each
##' element of the test set. This indicates the variability in the
##' model fitting process.
##'
##' @param model The tidymodels model to use for the fit
##' @param train The training data on which to perform the fits
##' @param test The test set, where each model (from the bootstrap resamples)
##' is used to make a prediction for the items in the test set
##' @param resamples_from_train The bootstrap resamples object used to specify
##' which subsets of train are used to train the models.
##' @param recipe The recipe (preprocessing steps) to apply before training
##' _any_ of the models. (i.e. the same preprocessing steps are applied before
##' fitting the models in the bootstrap resamples.
##' @return A list of two items. The first ("results") is a tibble with
##' the test set, containing predictors along with a model_id column
##' indicating which bootstrap resample was used to fit that model. The
##' second ("removals") is a list of predictors removed due to near-zero
##' variance in the primary fit.
predict_resample <- function(model, train, test, resamples_from_train, recipe)
{
    ## Create the workflow for this model
    workflow <- workflow() %>%
        add_model(model) %>%
        add_recipe(recipe)

    ## Set the control to extract the fitted model for each resample
    ## Turns out you don't need to run extract_fit_parsnip. Surely
    ## there is a way not to call the identity function!
    ctrl_rs <- control_resamples(
        extract = function (x) x
    )

    ## Perform an independent fit on each bootstrapped resample,
    ## extraclting the fit objects
    bootstrap_fits <- workflow %>%
        fit_resamples(resamples_from_train, control = ctrl_rs) %>%
        pull(.extracts) %>%
        map(~ .x %>% pluck(".extracts", 1))
    
    ## Perform one fit on the entire training dataset. This is the
    ## single (no cross-validation here) main fit of the model on the
    ## entire training set.
    primary_fit <- workflow %>%
        fit(data = train)

    ## Use the primary model to predict the test set
    primary_pred <- primary_fit %>%
        augment(new_data = test) %>%
        mutate(model_id = as.factor("primary"))

    primary_removals <- primary_fit %>%
        extract_recipe() %>%
        pluck("steps", 2, "removals")
    
    ## For each bleeding model, predict the probabilities for
    ## the test set and record the model used to make the
    ## prediction in model_id
    bootstrap_pred <- list(
        n = seq_along(bootstrap_fits),
        f = bootstrap_fits
    ) %>%
        pmap(function(n, f)
        {
            f %>%
                augment(new_data = test) %>%
                mutate(model_id = as.factor(n))
        }) %>%
        list_rbind()

    ## Bind together the primary and bootstrap fits
    pred <- bind_rows(primary_pred, bootstrap_pred)

    list(results = pred, removals = primary_removals) 
}

'''
