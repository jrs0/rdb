#include <gtest/gtest.h>
#include "../clinical_code.hpp"

/// Check that string can be inserted and then read
TEST(ClinicalCode, NullOnDefaultConstruction) {
    ClinicalCode clinical_code;
    EXPECT_TRUE(clinical_code.null());
}

/// Note this test relies on the current version of
/// the groups in the codes files
TEST(ClinicalCodeGroup, Contains) {

    auto lookup{new_string_lookup()};
    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml", lookup};
    ClinicalCodeGroup group{"acs_stemi", lookup};

    {
	auto code{parser.parse_diagnosis("I21.0")};
        EXPECT_TRUE(group.contains(code));
    }

    {
	auto code{parser.parse_diagnosis("A000")};
	EXPECT_FALSE(group.contains(code));
    }

    // Check the null code is not in the group
    EXPECT_FALSE(group.contains(ClinicalCode{}));
}
