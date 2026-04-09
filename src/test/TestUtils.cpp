#include"TestUtils.hpp"
#include"VendorRasterTest.hpp"
#include"HighPointTest.hpp"

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

    Raster<csm_t> applyDefaultCsm(const InputData& input)
    {
        namespace fs = std::filesystem;
        auto [algo, testCase] = getPipelineSectionTestCase("CSMCreation");
        fs::path path = getFullFilename(algo, testCase, input, "tif");
        if (!fs::exists(path)) {
            throw std::runtime_error("File does not exist: " + path.string());
        }
        return Raster<csm_t>{ path.string() };
    }

    Raster<csm_t> applyDefaultCsmPostProcess(const InputData& input)
    {
        auto [algo, testCase] = getPipelineSectionTestCase("CSMPostProcessing");
        if (algo == "DoNothingCsm") {
            return applyDefaultCsm(input);
        }
        else {
            throw std::runtime_error("Additional code needed to support other csm post-processing algorithms in default pipeline");
        }
    }

    std::vector<IDedTao> applyDefaultTreeIdentification(const InputData& input)
    {
        auto [algo, testCase] = getPipelineSectionTestCase("TreeIdentification");
        if (algo == "HighPoints") {
            csm_t minHeight = testCase.params.at("MinHeight").as<csm_t>();
            coord_t minDist = testCase.params.at("MinDist").as<coord_t>();
            return applyHighPoint(input, testCase);
        }
        else {
            throw std::runtime_error("Additional code needed to support other tree identification algorithms in default pipeline");
        }
    }


    std::vector<std::filesystem::path> TestFileSystemNoIO::listDirectory(const std::filesystem::path& dirPath) const
    {
        auto it = _dirContents.find(dirPath);
        if (it != _dirContents.end()) {
            return it->second;
        }
        return {};
    }
    bool TestFileSystemNoIO::isDirectory(const std::filesystem::path& path) const
    {
        return _directories.find(path) != _directories.end();
    }
    bool TestFileSystemNoIO::isRegularFile(const std::filesystem::path& path) const
    {
        return _files.find(path) != _files.end();
    }
    void TestFileSystemNoIO::addDirectory(const std::filesystem::path& dirPath)
    {
        _directories.insert(dirPath);
    }
    void TestFileSystemNoIO::addFile(const std::filesystem::path& filePath)
    {
        _files.insert(filePath);
        std::filesystem::path parentDir = filePath.parent_path();
        _dirContents[parentDir].push_back(filePath);
    }
    void TestFileSystemNoIO::clear()
    {
        _directories.clear();
        _files.clear();
        _dirContents.clear();
    }

    std::string MockDemOpener::fileNameFromAlignment(const Alignment& a)
    {
        std::stringstream ss;
        ss << "raster_" << a.xmin() << "_" << a.ymin() << "_" << a.xres() << "_" << a.yres() << "_" << a.ncol() << "_" << a.nrow() << "_" << a.crs().getEPSG() << ".tif";
        return ss.str();
    }

    DemParameter::DemFileAlignment MockDemOpener::operator()(const std::filesystem::path& f) const
    {
        //f might be a directory or a file
        //in either case, we extract the name of the file/dir itself, excluding parents
        //if the pattern is generated by fileNameFromAlignment, we can extract the alignment info
        //otherwise, through an error
        std::filesystem::path filename = f.filename();
        std::string fnameStr = filename.string();
        coord_t xmin, ymin, xres, yres;
        rowcol_t ncol, nrow;
        char epsgStr[20];
        int matched = sscanf_s(fnameStr.c_str(), "raster_%lf_%lf_%lf_%lf_%d_%d_%[^.].tif",
            &xmin, &ymin, &xres, &yres, &ncol, &nrow, epsgStr, static_cast<unsigned int>(20));
        if (matched != 7) {
            throw std::runtime_error("MockDemOpener cannot parse filename: " + fnameStr);
        }
        return DemParameter::DemFileAlignment{
            f,
            Alignment{
                xmin, ymin,
                ncol, nrow,
                xres, yres,
                CoordRef(epsgStr)
            }
        };
    }


    std::string MockLasOpener::fileNameFromLasExtent(const LasExtent& le)
    {
        std::stringstream ss;
        ss << "las_" << le.xmin() << "_" << le.ymin() << "_" << le.xmax() << "_" << le.ymax() << "_" << le.nPoints() << "_" << le.crs().getEPSG() << ".laz";
        return ss.str();
    }
    LasFileParameter::LasFileExtent MockLasOpener::operator()(const std::filesystem::path& f) const
    {
        //f might be a directory or a file
        //in either case, we extract the name of the file/dir itself, excluding parents
        //if the pattern is generated by fileNameFromLasExtent, we can extract the las extent info
        //otherwise, through an error
        std::filesystem::path filename = f.filename();
        std::string fnameStr = filename.string();
        coord_t xmin, ymin, xmax, ymax;
        uint64_t nPoints;
        char epsgStr[20];
        int matched = sscanf_s(fnameStr.c_str(), "las_%lf_%lf_%lf_%lf_%llu_%[^.].laz",
            &xmin, &ymin, &xmax, &ymax, &nPoints, epsgStr, static_cast<unsigned int>(20));
        if (matched != 6) {
            throw std::runtime_error("MockLasOpener cannot parse filename: " + fnameStr);
        }
        return LasFileParameter::LasFileExtent{
            f,
            LasExtent{
                Extent(xmin, xmax, ymin, ymax),
                nPoints
            }
        };
    }

    TestParameterGetter::TestParameterGetter(const std::filesystem::path& yamlPath)
    {
        YAML::Node yaml = YAML::LoadFile(yamlPath.string());

        for (const auto& groupNode : yaml) {
            ParameterGroup group;
            group.name = groupNode.first.as<std::string>();

            for (const auto& entry : groupNode.second) {
                std::map<std::string, YAML::Node> params;
                if (entry.IsMap()) {
                    for (const auto& param : entry) {
                        params[param.first.as<std::string>()] = param.second;
                    }
                }
                group.entries.push_back(std::move(params));
            }

            if (!group.entries.empty()) {
                groups_.push_back(std::move(group));
            }
        }
    }

    std::vector<std::string> TestParameterGetter::getDefaultTestParameters() const
    {
        std::vector<std::string> argv;
        for (const auto& group : groups_) {
            if (!group.entries.empty()) {
                appendParamsToArgv(argv, group.entries[0]);
            }
        }
        return argv;
    }

    std::vector<std::vector<std::string>> TestParameterGetter::getAllTestParameter() const
    {
        std::vector<std::vector<std::string>> allCases;
        allCases.push_back(getDefaultTestParameters());

        for (size_t groupIdx = 0; groupIdx < groups_.size(); ++groupIdx) {
            const auto& varyingGroup = groups_[groupIdx];

            for (size_t entryIdx = 1; entryIdx < varyingGroup.entries.size(); ++entryIdx) {
                std::vector<std::string> argv;

                for (size_t i = 0; i < groups_.size(); ++i) {
                    if (i == groupIdx) {
                        appendParamsToArgv(argv, varyingGroup.entries[entryIdx]);
                    }
                    else {
                        if (!groups_[i].entries.empty()) {
                            appendParamsToArgv(argv, groups_[i].entries[0]);
                        }
                    }
                }

                allCases.push_back(std::move(argv));
            }
        }

        return allCases;
    }

    void TestParameterGetter::appendParamsToArgv(std::vector<std::string>& argv, const std::map<std::string, YAML::Node>& params) const
    {
        for (const auto& [key, value] : params) {
            if (value.IsSequence()) {
                std::stringstream ss;
                for (size_t i = 0; i < value.size(); ++i) {
                    if (i > 0) ss << ",";
                    ss << value[i].as<std::string>();
                }
                argv.push_back("--" + key + "=" + ss.str());
            }
            else {
                argv.push_back("--" + key + "=" + value.as<std::string>());
            }
        }
    }

}