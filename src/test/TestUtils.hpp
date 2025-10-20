#pragma once
#ifndef LP_TEST_UTILS_HPP
#define LP_TEST_UTILS_HPP

#include"test_pch.hpp"

namespace lapis {
    inline std::filesystem::path testdata_repository_folder;
    inline std::filesystem::path testdata_generated_folder;

    //Lapis does not promise exact equality with lidR; so I only test for ballpark equality
    constexpr coord_t LAPIS_TEST_SMALL_EPSILON = 0.5;
    constexpr coord_t LAPIS_TEST_LARGE_EPSILON = 1;

    void initTestEnvVars();
    std::filesystem::path findFileInEitherFolder(const std::string& filename);

    struct TestCase {
        std::map<std::string, YAML::Node> params;
    };

    std::vector<TestCase> getTests(const std::string& section);

    struct InputData {
        std::string shortName;
        std::filesystem::path lazFile;
        std::vector<std::filesystem::path> tifFile;
    };

    std::vector<InputData> getInputDataList();

    std::filesystem::path getFullFilename(const std::string& algoname, const TestCase& testCase, const InputData& input, const std::string& extension);

    LidarPointVector applyDefaultNormalization(const InputData& input);

    Raster<csm_t> applyDefaultCsm(const InputData& input);

    Raster<csm_t> applyDefaultCsmPostProcess(const InputData& input);
}

#endif