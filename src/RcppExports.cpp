// Generated by using Rcpp::compileAttributes() -> do not edit by hand
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include <Rcpp.h>

using namespace Rcpp;

#ifdef RCPP_USE_GLOBAL_ROSTREAM
Rcpp::Rostream<true>&  Rcpp::Rcout = Rcpp::Rcpp_cout_get();
Rcpp::Rostream<false>& Rcpp::Rcerr = Rcpp::Rcpp_cerr_get();
#endif

// test_cpp
int test_cpp(int thing);
RcppExport SEXP _rdb_test_cpp(SEXP thingSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< int >::type thing(thingSEXP);
    rcpp_result_gen = Rcpp::wrap(test_cpp(thing));
    return rcpp_result_gen;
END_RCPP
}
// make_acs_dataset
void make_acs_dataset(const Rcpp::CharacterVector& config_path_chr);
RcppExport SEXP _rdb_make_acs_dataset(SEXP config_path_chrSEXP) {
BEGIN_RCPP
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const Rcpp::CharacterVector& >::type config_path_chr(config_path_chrSEXP);
    make_acs_dataset(config_path_chr);
    return R_NilValue;
END_RCPP
}
// try_connect
Rcpp::List try_connect(const Rcpp::CharacterVector& dsn_character, const Rcpp::CharacterVector& query_character);
RcppExport SEXP _rdb_try_connect(SEXP dsn_characterSEXP, SEXP query_characterSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const Rcpp::CharacterVector& >::type dsn_character(dsn_characterSEXP);
    Rcpp::traits::input_parameter< const Rcpp::CharacterVector& >::type query_character(query_characterSEXP);
    rcpp_result_gen = Rcpp::wrap(try_connect(dsn_character, query_character));
    return rcpp_result_gen;
END_RCPP
}
// parse_code
void parse_code(const Rcpp::CharacterVector& icd10_file_character, const Rcpp::CharacterVector& code_character);
RcppExport SEXP _rdb_parse_code(SEXP icd10_file_characterSEXP, SEXP code_characterSEXP) {
BEGIN_RCPP
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const Rcpp::CharacterVector& >::type icd10_file_character(icd10_file_characterSEXP);
    Rcpp::traits::input_parameter< const Rcpp::CharacterVector& >::type code_character(code_characterSEXP);
    parse_code(icd10_file_character, code_character);
    return R_NilValue;
END_RCPP
}

static const R_CallMethodDef CallEntries[] = {
    {"_rdb_test_cpp", (DL_FUNC) &_rdb_test_cpp, 1},
    {"_rdb_make_acs_dataset", (DL_FUNC) &_rdb_make_acs_dataset, 1},
    {"_rdb_try_connect", (DL_FUNC) &_rdb_try_connect, 2},
    {"_rdb_parse_code", (DL_FUNC) &_rdb_parse_code, 2},
    {NULL, NULL, 0}
};

RcppExport void R_init_rdb(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
