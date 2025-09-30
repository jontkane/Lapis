#include"TestUtils.hpp"
#include"VendorRasterTest.hpp"

namespace lapis {
    void initTestEnvVars() {
        testdata_repository_folder = TESTDATA_INPUT_DIR;
        testdata_generated_folder = TESTDATA_OUTPUT_DIR;
    }
    std::filesystem::path findFileInEitherFolder(const std::string& filename)
    {
        namespace fs = std::filesystem;
        fs::path input_path = testdata_repository_folder / filename;
        fs::path output_path = testdata_generated_folder / filename;
        if (fs::exists(input_path)) {
            return input_path;
        } else if (fs::exists(output_path)) {
            return output_path;
        } else {
            throw std::runtime_error("File not found in either folder: " + filename);
        }
    }

    std::vector<TestCase> getTests(const std::string& section)
    {
        namespace fs = std::filesystem;
        fs::path testYaml = findFileInEitherFolder("AlgorithmParamsTestValues.yaml");
        YAML::Node yaml = YAML::LoadFile(testYaml.string());
        YAML::Node sectionNode = yaml["Tests"][section];
        std::vector<TestCase> tests;

        std::vector<std::string> paramNames;
        std::vector<std::vector<YAML::Node>> paramValues;
        for (const auto& node : sectionNode) {
            paramNames.push_back(node.first.as<std::string>());
            std::vector<YAML::Node> values;
            for (const auto& v : node.second) {
                values.push_back(v);
            }
            paramValues.push_back(std::move(values));
        }

        std::vector<size_t> indices(paramValues.size(), 0);
        while (true) {
            std::map<std::string, YAML::Node> paramSet;
            for (size_t i = 0; i < paramNames.size(); ++i) {
                paramSet[paramNames[i]] = paramValues[i][indices[i]];
            }
            tests.emplace_back(paramSet);

            size_t k = paramValues.size();
            while (k-- > 0) {
                if (++indices[k] < paramValues[k].size()) {
                    break;
                }
                indices[k] = 0;
            }
            if (k == static_cast<size_t>(-1)) break;
        }

        return tests;
    }

    std::vector<InputData> getInputDataList()
    {
        namespace fs = std::filesystem;
        fs::path inputYaml = findFileInEitherFolder("tif_laz_associations.yaml");
        YAML::Node yaml = YAML::LoadFile(inputYaml.string());

        std::vector<InputData> inputDataList;
        for (const auto& item : yaml) {
            InputData inputData;
            inputData.shortName = item["shortname"].as<std::string>();
            inputData.lazFile = testdata_repository_folder / item["laz"].as<std::string>();
            for (const auto& tif : item["tif"]) {
                inputData.tifFile.push_back(testdata_repository_folder / tif.as<std::string>());
            }
            inputDataList.push_back(inputData);
        }
        return inputDataList;
    }

    std::filesystem::path getFullFilename(const std::string& algoname, const TestCase& testCase, const InputData& input, const std::string& extension)
    {
        std::stringstream ss;
        ss << algoname << "_" << input.shortName;
        for (const auto& param : testCase.params) {
            ss << "_" << param.second.as<std::string>();
        }
        ss << "." << extension;
        return findFileInEitherFolder(ss.str());
    }

    namespace {
        struct PipelineAlgo {
            std::string name;
            TestCase testCase;
        };

        PipelineAlgo getPipelineSectionTestCase(const std::string& section) {
            namespace fs = std::filesystem;
            fs::path testYaml = lapis::findFileInEitherFolder("AlgorithmParamsTestValues.yaml");
            YAML::Node yaml = YAML::LoadFile(testYaml.string());
            YAML::Node sectionNode = yaml["Pipeline"][section];
            if (!sectionNode || sectionNode.size() != 1) {
                throw std::runtime_error("Pipeline section '" + section + "' missing or does not have exactly one algorithm.");
            }
            auto algoIt = sectionNode.begin();
            std::string algoName = algoIt->first.as<std::string>();
            std::map<std::string, YAML::Node> params;
            for (const auto& param : algoIt->second) {
                params[param.first.as<std::string>()] = param.second;
            }
            return PipelineAlgo{ algoName, TestCase{params} };
        }
    }

    LidarPointVector applyDefaultNormalization(const InputData& input)
    {
        namespace fs = std::filesystem;
        auto [algo, testCase] = getPipelineSectionTestCase("Normalization");
        fs::path path = getFullFilename(algo, testCase, input, "laz");
        if (!fs::exists(path)) {
            throw std::runtime_error("File does not exist: " + path.string());
        }
        LasReader l{ path.string() };
        return l.getPoints(l.nPoints());
    }


}