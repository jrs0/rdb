#ifndef RFACTOR_HPP
#define RFACTOR_HPP

#include <Rcpp.h>

class RFactor {
public:
    void push_back(const std::string & value) {
	// R factor levels must start at 1
	factor_.push_back(lookup_.insert_string(value) + 1);	
    }
    const auto & get() {
	auto strings{lookup_.strings()};
        Rcpp::CharacterVector levels(strings.begin(), strings.end());
        factor_.attr("levels") = levels;
	factor_.attr("class") = "factor";
        return factor_;
    }
private:
    Rcpp::IntegerVector factor_;
    StringLookup lookup_;
};

#endif
