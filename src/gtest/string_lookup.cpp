#include <gtest/gtest.h>
#include "string_lookup.h"

/// Check that string can be inserted and then read
TEST(StringLookupTest, ReadAfterInsert) {

    StringLookup lookup;
    auto id1{lookup.insert_string("Hello World!")};
    auto id2{lookup.insert_string("Another String")};

    // Check ids are not equal
    EXPECT_NE(id1, id2);

    // Check the strings
    EXPECT_EQ("Hello World!", lookup.at(id1));
    EXPECT_EQ("Another String", lookup.at(id2));
}
