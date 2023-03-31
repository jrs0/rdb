#include <gtest/gtest.h>
#include "../clinical_code.hpp"

/// Check that string can be inserted and then read
TEST(ClinicalCode, NullOnDefaultConstruction) {
    ClinicalCode clinical_code;
    EXPECT_TRUE(clinical_code.null());
}

TEST(ClinicalCodeGroup, Contains) {

    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml"};
    auto code{parser.parse_diagnosis("I21.0")};
    ClinicalCodeGroup group{
}
