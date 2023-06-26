#include <gtest/gtest.h>
#include "table.h"

/// Throws exception if first row not created
TEST(CountTable, NoFirstRow)
{
    CountTable table;
    EXPECT_THROW(table.increment_count(0), CountTable::CallAddRowFirst);
}

/// Check values are correctly added to a single column
TEST(CountTable, OneColumn)
{
    CountTable table;
    table.add_row();
    table.increment_count(0);
    
    // Check state is a single column with a single value
    EXPECT_EQ(table.columns().size(), 1); // 1 column
    EXPECT_EQ(table.columns().at(0).size(), 1); // 1 row
    EXPECT_EQ(table.columns().at(0).at(0), 1); // Value is 1

    // Increment the same value again and check
    table.increment_count(0);
    EXPECT_EQ(table.columns().size(), 1); // 1 column
    EXPECT_EQ(table.columns().at(0).size(), 1); // 1 row
    EXPECT_EQ(table.columns().at(0).at(0), 2); // Value is 2

    // Add a new row, and check the two values
    table.add_row();
    table.increment_count(0);
    EXPECT_EQ(table.columns().size(), 1); // 1 column
    EXPECT_EQ(table.columns().at(0).size(), 2); // 2 rows
    EXPECT_EQ(table.columns().at(0).at(0), 2); // Value is 2
    EXPECT_EQ(table.columns().at(0).at(1), 1); // Value is 1
}

/// Check two columns where one is added later
TEST(CountTable, TwoColumns)
{
    CountTable table;

    // Make a single column with two values
    table.add_row();
    table.increment_count(0);
    table.add_row();
    table.increment_count(0);
    table.increment_count(0);

    // Add a second column 
    table.add_row();
    table.increment_count(1);

    // Expect both columns to be length three
    EXPECT_EQ(table.columns().size(), 2); // 2 columns 
    EXPECT_EQ(table.columns().at(0).size(), 3); // 3 rows
    EXPECT_EQ(table.columns().at(1).size(), 3); // 3 rows

    // Check column values
    EXPECT_EQ(table.columns().at(0), (std::vector<long long>{1, 2, 0}));
    EXPECT_EQ(table.columns().at(1), (std::vector<long long>{0, 0, 1}));
}