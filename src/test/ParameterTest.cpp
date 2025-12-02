#include"ParameterTest.hpp"

namespace lapis {

    TEST(ParameterTest, FileSpecifierCrawlerTest) {
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
        //5. openers can be expected to throw an error when fed unsupported files or folders, this should be sinlently caught and ignored

        //these will each be added in three ways: once as a direct file specifier, once as part of a folder specifier, and once as part of a wildcard specifier
        std::vector<Alignment> expectedAligns = {Alignment(0,0,1,1,10,10,"EPSG:26910"),
                                                 Alignment(10,10,11,11,10,10,"EPSG:26910"),
                                                 Alignment(20,20,21,21,10,10,"EPSG:26910"),
                                                 Alignment(30,30,31,31,10,10,"EPSG:26910") };

        MockDemOpener opener;
        TestFileSystemNoIO fileSystem;
        namespace fs = std::filesystem;
        fs::path baseDir = "C:/testdata";
        fileSystem.addDirectory(baseDir);
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

        //each element of extectedAligns should appear exactly three times in foundAligns
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
}