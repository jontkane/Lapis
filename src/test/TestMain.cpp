#include"test_pch.hpp"
#include"TestUtils.hpp"

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    lapis::initTestEnvVars();
    return RUN_ALL_TESTS();
}