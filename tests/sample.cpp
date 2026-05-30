#include <fmt/core.h>
#include <gtest/gtest.h>

TEST(SampleTest, Addition) {
    EXPECT_EQ(2 + 2, 4);
    fmt::print("Test executed successfully\n");
}
