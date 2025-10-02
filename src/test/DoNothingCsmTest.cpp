#include"DoNothingCsmTest.hpp"
#include"..\algorithms\DoNothingCsm.hpp"

namespace lapis {

    const std::string doNothingCsmName = "DoNothingCsm";

    Raster<csm_t> applyDoNothingCsm(const InputData& input, const TestCase& testCase)
    {
        Raster<csm_t> defaultCsm = applyDefaultCsm(input);
        DoNothingCsm algo;
        return algo.postProcess(defaultCsm);
    }

    TEST(CsmPostProcessorTests, DoNothingCsmTest) {
        for (const InputData& input : getInputDataList()) {
            for (const TestCase& testCase : getTests(doNothingCsmName)) {

                //because the do-nothing algorithm does nothing, we can just compare to the default csm

                Raster<csm_t> truthRaster = applyDefaultCsm(input);

                Raster<csm_t> testRaster = applyDoNothingCsm(input, testCase);
                ASSERT_EQ(testRaster.ncol(), truthRaster.ncol());
                ASSERT_EQ(testRaster.nrow(), truthRaster.nrow());
                ASSERT_EQ(testRaster.xres(), truthRaster.xres());
                ASSERT_EQ(testRaster.yres(), truthRaster.yres());
                ASSERT_EQ(testRaster.xOrigin(), truthRaster.xOrigin());
                ASSERT_EQ(testRaster.yOrigin(), truthRaster.yOrigin());


                for (cell_t cell : CellIterator(truthRaster)) {
                    EXPECT_EQ(testRaster.atCellUnsafe(cell).has_value(), truthRaster.atCellUnsafe(cell).has_value());
                    if (testRaster.atCellUnsafe(cell).has_value()) {
                        //normally I would use an epsilon, but this algorithm really should do *nothing*, so true double equality is appropriate
                        EXPECT_EQ(testRaster.atCellUnsafe(cell).value(), truthRaster.atCellUnsafe(cell).value());
                    }
                }
            }
        }
    }
}
