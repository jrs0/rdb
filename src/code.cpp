#include <Rcpp.h>
using namespace Rcpp;

//[[Rcpp::export]]
int test_cpp(int thing) {
    return thing + 1;
}
