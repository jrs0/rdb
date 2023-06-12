#include <gtest/gtest.h>
#include "sql_types.h"

TEST(Timestamp, DefaultConstructNull) {
    Timestamp t;
    EXPECT_TRUE(t.null());
}

TEST(Timestamp, SetInitialTimestamp) {
    Timestamp t{300};
    EXPECT_EQ(t.read(), 300);
}

TEST(Timestamp, OffsetBackwards) {
    Timestamp t{600};
    auto x{t + -200};
    EXPECT_EQ(x.read(), 400);
}

TEST(Timestamp, OffsetForwards) {
    Timestamp t{600};
    auto x{t + 200};
    EXPECT_EQ(x.read(), 800);
}

TEST(Timestamp, LargeOffsetBackwards) {
    Timestamp t{1600473600}; // 2020-09-19  
    auto x{t + -365*24*60*60}; // 1 year
    EXPECT_EQ(x.read(), 1568937600);
}

TEST(Timestamp, LargeOffsetForwards) {
    Timestamp t{1600473600}; // 2020-09-19  
    auto x{t + 365*24*60*60}; // 1 year
    EXPECT_EQ(x.read(), 1632009600);
}
