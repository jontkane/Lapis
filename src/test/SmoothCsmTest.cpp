#include"SmoothCsmTest.hpp"
#include"..\algorithms\SmoothCsm.hpp"

namespace lapis {
    const std::string smoothCsmName = "SmoothCsm";
    const std::string smoothWindowParamName = "SmoothWindow";


    Raster<csm_t> applySmoothCsm(const InputData& input, const TestCase& testCase)
    {
        Raster<csm_t> defaultCsm = applyDefaultCsm(input);
        int smoothWindow = testCase.params.at(smoothWindowParamName).as<int>();
        SmoothCsm algo(smoothWindow);
        return algo.postProcess(defaultCsm);
    }
    TEST(CsmPostProcessorTests, SmoothCsmTest) {
        for (const InputData& input : getInputDataList()) {
            for (const TestCase& testCase : getTests(smoothCsmName)) {
                Raster<csm_t> testRaster = applySmoothCsm(input, testCase);
                Raster<csm_t> truthRaster = Raster<csm_t>(getFullFilename(smoothCsmName, testCase, input, "tif").string());
                ASSERT_EQ(testRaster.ncol(), truthRaster.ncol());
                ASSERT_EQ(testRaster.nrow(), truthRaster.nrow());
                ASSERT_EQ(testRaster.xres(), truthRaster.xres());
                ASSERT_EQ(testRaster.yres(), truthRaster.yres());
                ASSERT_EQ(testRaster.xOrigin(), truthRaster.xOrigin());
                ASSERT_EQ(testRaster.yOrigin(), truthRaster.yOrigin());


                //the easiest R function to compare against implicitly fills in NAs; so I will not expect has_value() to be equal when testRaster is NoData
                for (cell_t cell : CellIterator(truthRaster)) {
                    if (testRaster.atCellUnsafe(cell).has_value()) {
                        EXPECT_TRUE(truthRaster.atCellUnsafe(cell).has_value());
                        EXPECT_NEAR(testRaster.atCellUnsafe(cell).value(), truthRaster.atCellUnsafe(cell).value(), LAPIS_TEST_SMALL_EPSILON);
                    }
                }
            }
        }
    }

}