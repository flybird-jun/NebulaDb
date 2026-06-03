#include <gtest/gtest.h>
#include "common.h"

TEST(CommonTest, HelloReturnsExpectedString) {
    EXPECT_EQ(nebuladb::common::Hello(), "Hello from NebulaDb!");
}
