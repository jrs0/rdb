#include <gtest/gtest.h>
#include "../clinical_code.hpp"

/// Check that string can be inserted and then read
TEST(ClinicalCode, NullOnDefaultConstruction) {
    ClinicalCode clinical_code;
    EXPECT_TRUE(clinical_code.null());
}

TEST(ClinicalCode, ParseInvalidCode) {
    auto lookup{new_string_lookup()};
    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml", lookup};
    auto code{parser.parse(CodeType::Diagnosis, "K85X")};
    EXPECT_FALSE(code.null());
    EXPECT_TRUE(code.invalid());
    EXPECT_EQ(code.name(lookup), "K85X");
}

/// Parse a mixture of valid and invalid codes
TEST(ClinicalCode, ParseValidInvalidCodes) {
    auto lookup{new_string_lookup()};
    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml", lookup};

    /// Valid
    {
	auto code{parser.parse(CodeType::Diagnosis, "I210")};
	// EXPECT_FALSE(code.null());
	// EXPECT_FALSE(code.invalid());
	// EXPECT_EQ(code.name(lookup), "I21.0");
    }
    
    /// Invalid
    {
	auto code{parser.parse(CodeType::Diagnosis, "K85X")};
	// EXPECT_FALSE(code.null());
	// EXPECT_TRUE(code.invalid());
	// EXPECT_EQ(code.name(lookup), "K85X");
    }

    /// Valid
    {
	auto code{parser.parse(CodeType::Diagnosis, "D73.1")};
	// EXPECT_FALSE(code.null());
	// EXPECT_FALSE(code.invalid());
	// EXPECT_EQ(code.name(lookup), "D37.1");
    }
    
    /// Invalid
    {
	auto code{parser.parse(CodeType::Diagnosis, "abcd")};
	// EXPECT_FALSE(code.null());
	// EXPECT_TRUE(code.invalid());
	// EXPECT_EQ(code.name(lookup), "abcd");
    }
    lookup->print();
}


/// Note this test relies on the current version of
/// the groups in the codes files
TEST(ClinicalCodeGroup, Contains) {

    auto lookup{new_string_lookup()};
    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml", lookup};
    ClinicalCodeGroup group{"acs_stemi", lookup};

    {
	auto code{parser.parse(CodeType::Diagnosis, "I21.0")};
        EXPECT_TRUE(group.contains(code));
    }

    {
	auto code{parser.parse(CodeType::Diagnosis, "A000")};
	EXPECT_FALSE(group.contains(code));
    }

    // Check the null code is not in the group
    EXPECT_FALSE(group.contains(ClinicalCode{}));
}
