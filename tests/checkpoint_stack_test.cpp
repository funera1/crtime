#include <gtest/gtest.h>

int start() {
}

TEST(TestCase, sum) {
    EXPECT_EQ(2, sum(1, 1));
}