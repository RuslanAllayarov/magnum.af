#include <gtest/gtest.h>
#include <math.h>
#include "../../../src/material.cpp"
 
// Exemplary unit test
TEST(ParamInitTest, mu0) {
    Material material;
    ASSERT_EQ(4e-7 * M_PI, Constants.mu0);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
