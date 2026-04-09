#include"ParameterTest.hpp"

namespace lapis {

    TEST(ParameterTest, FileSpecifierCrawlerTest) {
        MockDemOpener opener;
        TestFileSystemNoIO fileSystem;
        namespace fs = std::filesystem;

        // Test 1: Basic functionality - files added three ways (direct, folder, wildcard)
        {
            FileSpecifierSet fileSpecSet{
                "Test File Specifier",
                "test-file-spec",
                "A test file specifier for unit testing.",
                { "*.txt", "*.md" },
                nullptr
            };

            //1. every file should be inspected
            //2. every folder should be inspected
            //3. the contents of every folder should be inspected, recursively
            //4. specifiers such as folder/*.tif should be handled as if they were non-recursive folder inspections with an extension filter
            //5. openers can be expected to throw an error when fed unsupported files or folders, this should be silently caught and ignored

            fs::path baseDir = "C:/testdata";
            fileSystem.clear();
            fileSystem.addDirectory(baseDir);

            //these will each be added in three ways: once as a direct file specifier, once as part of a folder specifier, and once as part of a wildcard specifier
            std::vector<Alignment> expectedAligns = {Alignment(0,0,1,1,10,10,"EPSG:26910"),
                                                     Alignment(10,10,11,11,10,10,"EPSG:26910"),
                                                     Alignment(20,20,21,21,10,10,"EPSG:26910"),
                                                     Alignment(30,30,31,31,10,10,"EPSG:26910") };

            for (const Alignment& align : expectedAligns) {
                fs::path filePath = baseDir / MockDemOpener::fileNameFromAlignment(align);
                fileSystem.addFile(filePath);
                fileSpecSet.addSpecifier(filePath.string());
            }
            fileSpecSet.addSpecifier(baseDir.string());
            fileSpecSet.addSpecifier((baseDir.string() + "/*.tif"));

            //because the ESRI grid format means that rasters are sometimes folders, we want to make sure the crawler passes folder into the opener, instead of merely checking their contents
            Alignment folderAlign(40, 40, 41, 41, 10, 10, "EPSG:26910");
            fileSystem.addDirectory(baseDir / MockDemOpener::fileNameFromAlignment(folderAlign));
            fileSpecSet.addSpecifier((baseDir.string() + "/" + MockDemOpener::fileNameFromAlignment(folderAlign)));

            std::vector<DemParameter::DemFileAlignment> foundAligns = fileSpecSet.getFiles<MockDemOpener, DemParameter::DemFileAlignment>(opener, dynamic_cast<FileSystemWrapper*>(&fileSystem));

            //each element of expectedAligns should appear exactly three times in foundAligns
            //folderAlign should appear exactly once
            for (const Alignment& align : expectedAligns) {
                size_t count = 0;
                for (const DemParameter::DemFileAlignment& found : foundAligns) {
                    if (found.align == align) {
                        count++;
                    }
                }
                EXPECT_EQ(count, 3) << "Alignment " << MockDemOpener::fileNameFromAlignment(align) << " found " << count << " times instead of 3.";
            }
            size_t folderCount = 0;
            for (const DemParameter::DemFileAlignment& found : foundAligns) {
                if (found.align == folderAlign) {
                    folderCount++;
                }
            }
            EXPECT_EQ(folderCount, 1) << "Folder alignment found " << folderCount << " times instead of 1.";
        }

        // Mixed case extensions
        {
            FileSpecifierSet fileSpecSet{
                "Test File Specifier",
                "test-file-spec",
                "A test file specifier for unit testing.",
                { "*.tif" },
                nullptr
            };

            fileSystem.clear();
            fs::path baseDir = "C:/testdata";
            fileSystem.addDirectory(baseDir);

            Alignment align1(300, 300, 301, 301, 10, 10, "EPSG:26910");
            Alignment align2(310, 310, 311, 311, 10, 10, "EPSG:26910");

            std::string baseName1 = MockDemOpener::fileNameFromAlignment(align1);
            std::string baseName2 = MockDemOpener::fileNameFromAlignment(align2);
            
            baseName2.replace(baseName2.find(".tif"), 4, ".TIF");

            fileSystem.addFile(baseDir / baseName1);
            fileSystem.addFile(baseDir / baseName2);

            fileSpecSet.addSpecifier((baseDir.string() + "/*.tif"));

            std::vector<DemParameter::DemFileAlignment> foundAligns = fileSpecSet.getFiles<MockDemOpener, DemParameter::DemFileAlignment>(opener, dynamic_cast<FileSystemWrapper*>(&fileSystem));

            EXPECT_GE(foundAligns.size(), 1) << "Should find at least the .tif file";
        }

        // Special characters in filenames
        {
            FileSpecifierSet fileSpecSet{
                "Test File Specifier",
                "test-file-spec",
                "A test file specifier for unit testing.",
                { "*.tif" },
                nullptr
            };

            fileSystem.clear();
            fs::path baseDir = "C:/testdata";
            fileSystem.addDirectory(baseDir);

            std::string fileName1 = "file with spaces_400_400_1_1_10_10_EPSG26910.tif";
            fileSystem.addFile(baseDir / fileName1);

            std::string fileName2 = "file&name#test_410_410_1_1_10_10_EPSG26910.tif";
            fileSystem.addFile(baseDir / fileName2);

            fileSpecSet.addSpecifier(baseDir.string());

            //shouldn't crash or throw
            std::vector<DemParameter::DemFileAlignment> foundAligns = fileSpecSet.getFiles<MockDemOpener, DemParameter::DemFileAlignment>(opener, dynamic_cast<FileSystemWrapper*>(&fileSystem));
        }

        // Wildcard non-recursive behavior
        {
            FileSpecifierSet fileSpecSet{
                "Test File Specifier",
                "test-file-spec",
                "A test file specifier for unit testing.",
                { "*.tif" },
                nullptr
            };

            fileSystem.clear();
            fs::path baseDir = "C:/testdata";
            fileSystem.addDirectory(baseDir);
            fileSystem.addDirectory(baseDir / "subdir");

            Alignment topLevelAlign(500, 500, 501, 501, 10, 10, "EPSG:26910");
            Alignment subDirAlign(510, 510, 511, 511, 10, 10, "EPSG:26910");

            fileSystem.addFile(baseDir / MockDemOpener::fileNameFromAlignment(topLevelAlign));
            fileSystem.addFile(baseDir / "subdir" / MockDemOpener::fileNameFromAlignment(subDirAlign));

            fileSpecSet.addSpecifier((baseDir.string() + "/*.tif"));

            std::vector<DemParameter::DemFileAlignment> foundAligns = fileSpecSet.getFiles<MockDemOpener, DemParameter::DemFileAlignment>(opener, dynamic_cast<FileSystemWrapper*>(&fileSystem));

            EXPECT_EQ(foundAligns.size(), 1) << "Wildcard specifier should find only top-level file";
            EXPECT_EQ(foundAligns[0].align, topLevelAlign) << "Should find only the top-level file";
        }

        // Duplicate paths
        {
            FileSpecifierSet fileSpecSet{
                "Test File Specifier",
                "test-file-spec",
                "A test file specifier for unit testing.",
                { "*.tif" },
                nullptr
            };

            fileSystem.clear();
            fs::path baseDir = "C:/testdata";
            fileSystem.addDirectory(baseDir);

            Alignment align(600, 600, 601, 601, 10, 10, "EPSG:26910");
            fs::path filePath = baseDir / MockDemOpener::fileNameFromAlignment(align);
            fileSystem.addFile(filePath);

            fileSpecSet.addSpecifier(filePath.string());
            fileSpecSet.addSpecifier(baseDir.string());
            fileSpecSet.addSpecifier((baseDir.string() + "/*.tif"));

            std::vector<DemParameter::DemFileAlignment> foundAligns = fileSpecSet.getFiles<MockDemOpener, DemParameter::DemFileAlignment>(opener, dynamic_cast<FileSystemWrapper*>(&fileSystem));

            EXPECT_EQ(foundAligns.size(), 3) << "File should be found once per specifier method";
            for (const auto& found : foundAligns) {
                EXPECT_EQ(found.align, align) << "All found entries should match the expected alignment";
            }
        }
    }
}