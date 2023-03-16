##' The functions in this file can be used to obtain a blank OPCS4
##' codes configuration file.
##'
##'


opcs_chapter_names <- list (
    A = "Nervous System",
    B = "Endocrine System and Breast",
    C = "Eye",
    D = "Ear",
    E = "Respiratory Tract",
    F = "Mouth",
    G = "Upper Digestive System",
    H = "Lower Digestive System",
    J = "Other Abdominal Organs, Principally Digestive",
    K = "Heart",
    L = "Arteries and Veins",
    M = "Urinary",
    N = "Male Genital Organs",
    P = "Lower Female Genital Tract",
    Q = "Upper Female Genital Tract",
    R = "Female Genital Tract Associated with Pregnancy, Childbirth and the Puerperium",
    S = "Skin",
    T = "Soft Tissue",
    U = "Diagnostic Imaging, Testing and Rehabilitation",
    V = "Bones and Joints of Skull and Spine",
    W = "Other Bones and Joints",
    X = "Miscellaneous Operations",
    Y = "Subsidiary Classification of Methods of Operation",
    Z = "Subsidiary Classification of Sites of Operation",
)

##' Generate the nested chapter structure from the flat OPCS4
##' codes list
##'
##' This function accepts a text file, whitespace-separated
##' with two columns, containing the OPCS code in the first
##' column and the description in the second column
##'
##' OPCS codes have the general format Xnn[.n] where X is the
##' chapter letter, n is a numeric digit, and the third
##' digit along with the point may be omitted.
##' 
opcs_get_codes <- function(input_file_path, output_name = "opcs.yaml") {
    codes <- readr::read_delim(input_file_path, col_names = c("",""))
    
}


##' To facilite searching for codes in the configuration
##' file, it is important for each object (category or code)
##' to store a range of codes that it contains. R supports
##' lexicographical comparison of characters by default,
##' so all that is required is to store a pair representing
##' the start of the range and the end of the range. This
##' function adds an index key to the structure passed as
##' the argument.
##'
##' The index key represents the first allowable code in
##' the category. This function replaces the code key in
##' the leaf nodes with an index, which contains the code
##' at that level.
##'
##' @title Index a codes definition structure
##' @return The codes structure with indices (a nested list)
##' @param codes The input nested list of codes
icd10_index_codes <- function(codes)
{
    ## For the new codes structure
    result <- list()
    
    for (object in codes)
    {
        if (!is.null(object$category))
        {
            ## Process the child objects
            object$child <- icd10_index_codes(object$child)

            ## Check if the category is a code range or
            ## a code, meaning it starts with a capital
            ## letter followed by a number, e.g.
            ## A00-A03 or I22. If the category is not
            ## in this form, then copy the value of
            ## the first index one level down (this
            ## is valid because of the order of evaulation
            ## of this function -- inside to out).
            if (grepl("[A-Z][0-9]", object$category))
            {
                ## Store the index as a range (start
                ## and end inclusive).
                object$index <- object$category %>%
                    stringr::str_split("-") %>%
                    unlist()
            }
            else
            {
                ## Object is a chapter. Copy the first
                ## index from one level down, using the
                ## start and end values for the range.

                ## Before doing that, the indexes must
                ## be sorted
                ## Get the sorted order of this level 
                k <- object$child %>%
                    ## Get the first value (the starting
                    ## value) of each range and sort
                    ## by that
                    purrr::map(~ .x$index[[1]]) %>%
                    unlist() %>%
                    order()

                ## Use the index k to reorder the level
                reordered <- object$child[k]

                ## Compute the range start and end based on the
                ## reordered list
                N <- length(reordered)
                object$index <- c(
                    ## Start of first range
                    reordered %>% purrr::chuck(1, "index", 1), 
                    ## End of last range
                    reordered %>%
                    purrr::chuck(N, "index", 2)
                )
            }


        }
        else if (!is.null(object$code))
        {
            object$index <- object$code %>%
                stringr::str_replace_all("\\.", "")
        }
        else
        {
            stop("Expected category or codes key in codes definition object")
        }

        result <- c(result, list(object))
    }

    ## Return the copy of the structure with indices
    result
}
