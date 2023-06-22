#include <gtest/gtest.h>
#include "category.h"
#include "string_lookup.h"

TEST(TopLevelCategory, CacheSize) {
    auto lookup{new_string_lookup()};
    TopLevelCategory top_level_category{YAML::LoadFile("../../scripts/icd10.yaml"), lookup};

    EXPECT_EQ(top_level_category.cache_size(), 0);

    // // Parse a single valid code and check cache size increases
    top_level_category.parse("I210");
    EXPECT_EQ(top_level_category.cache_size(), 1); 
    top_level_category.parse("i21.0   ");
    EXPECT_EQ(top_level_category.cache_size(), 1); 
    top_level_category.parse("  I21.0 ");
    EXPECT_EQ(top_level_category.cache_size(), 1); 

    // Parse a second valid code and check cache size increases
    top_level_category.parse("A000");
    EXPECT_EQ(top_level_category.cache_size(), 2); 
    top_level_category.parse("a000   ");
    EXPECT_EQ(top_level_category.cache_size(), 2); 
    top_level_category.parse("  A00.0 ");
    EXPECT_EQ(top_level_category.cache_size(), 2); 

}