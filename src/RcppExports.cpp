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
// connect
int connect();
RcppExport SEXP _rdb_connect() {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    rcpp_result_gen = Rcpp::wrap(connect());
    return rcpp_result_gen;
END_RCPP
}

static const R_CallMethodDef CallEntries[] = {
    {"_rdb_test_cpp", (DL_FUNC) &_rdb_test_cpp, 1},
    {"_rdb_connect", (DL_FUNC) &_rdb_connect, 0},
    {NULL, NULL, 0}
};

RcppExport void R_init_rdb(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
