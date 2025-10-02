#include"FillCsmTest.hpp"
#include"..\algorithms\FillCsm.hpp"

namespace lapis {

    const std::string fillCsmName = "FillCsm";
    const std::string fillCsmNeighborsNeededParam = "NeighborsNeeded";
    const std::string fillCsmLookDistParam = "LookDist";

    Raster<csm_t> applyFillCsm(const InputData& input, const TestCase& testCase)
    {
        Raster<csm_t> defaultCsm = applyDefaultCsm(input);
        coord_t lookDist = testCase.params.at(fillCsmLookDistParam).as<coord_t>();
        int neighborsNeeded = testCase.params.at(fillCsmNeighborsNeededParam).as<int>();

        FillCsm algo{ neighborsNeeded, lookDist };
        return algo.postProcess(defaultCsm);
    }

    TEST(CsmPostProcessorTests, FillCsmTest) {
        //we don't have real truth for this one, because I'm not applying an algorithm present in lidR
        //so, we will test by heuristics:
        //1. the filled raster should have no missing values where the default raster had values
        //2. with the parameters actually used in production (neighbors=5, dist=6), at least half of the holes should be filled (this is not a true invariant, but should hold for the actual input data being tested)
        //3. the filled-in values should be within the min and max of the original raster
        //4. increasing look distance or decreasing neighbors needed should not increase the number of holes
        //5. reflecting the input should also reflect the output

        auto checkSameAlignment = [](const Raster<csm_t>& r1, const Raster<csm_t>& r2) {
            ASSERT_EQ(r1.xmin(), r2.xmin());
            ASSERT_EQ(r1.xmax(), r2.xmax());
            ASSERT_EQ(r1.ymin(), r2.ymin());
            ASSERT_EQ(r1.ymax(), r2.ymax());
            ASSERT_EQ(r1.nrow(), r2.nrow());
            ASSERT_EQ(r1.ncol(), r2.ncol());
            ASSERT_EQ(r1.xres(), r2.xres());
            ASSERT_EQ(r1.yres(), r2.yres());
            };

        for (const InputData& input : getInputDataList()) {
            //because I am not comparing against lidR, I am not using TestCase objects derived from the yaml.

            Raster<csm_t> defaultCsm = applyDefaultCsm(input);
            cell_t defaultHoles = 0;
            csm_t minVal = std::numeric_limits<csm_t>::max();
            csm_t maxVal = std::numeric_limits<csm_t>::lowest();

            TestCase testCase3_5;
            testCase3_5.params[fillCsmLookDistParam] = 3.0;
            testCase3_5.params[fillCsmNeighborsNeededParam] = 5;
            Raster<csm_t> filledCsm3_5 = applyFillCsm(input, testCase3_5);
            cell_t filledHoles3_5 = 0;
            checkSameAlignment(defaultCsm, filledCsm3_5);

            TestCase testCase6_5;
            testCase6_5.params[fillCsmLookDistParam] = 6.0;
            testCase6_5.params[fillCsmNeighborsNeededParam] = 5;
            Raster<csm_t> filledCsm6_5 = applyFillCsm(input, testCase6_5);
            cell_t filledHoles6_5 = 0;
            checkSameAlignment(defaultCsm, filledCsm6_5);

            TestCase testCase3_3;
            testCase3_3.params[fillCsmLookDistParam] = 3.0;
            testCase3_3.params[fillCsmNeighborsNeededParam] = 3;
            Raster<csm_t> filledCsm3_3 = applyFillCsm(input, testCase3_3);
            cell_t filledHoles3_3 = 0;
            checkSameAlignment(defaultCsm, filledCsm3_3);

            for (cell_t cell : CellIterator(defaultCsm)) {
                if (defaultCsm[cell].has_value()) {
                    if (defaultCsm[cell].value() < minVal) {
                        minVal = defaultCsm[cell].value();
                    }
                    if (defaultCsm[cell].value() > maxVal) {
                        maxVal = defaultCsm[cell].value();
                    }
                }
            }
            
            for (cell_t cell : CellIterator(defaultCsm)) {
                if (!defaultCsm[cell].has_value()) {
                    ++defaultHoles;
                    if (filledCsm3_5[cell].has_value()) {
                        ++filledHoles3_5;
                        EXPECT_GE(filledCsm3_5[cell].value(), minVal);
                        EXPECT_LE(filledCsm3_5[cell].value(), maxVal);
                    }
                    if (filledCsm6_5[cell].has_value()) {
                        ++filledHoles6_5;
                        EXPECT_GE(filledCsm6_5[cell].value(), minVal);
                        EXPECT_LE(filledCsm6_5[cell].value(), maxVal);
                    }
                    if (filledCsm3_3[cell].has_value()) {
                        ++filledHoles3_3;
                        EXPECT_GE(filledCsm3_3[cell].value(), minVal);
                        EXPECT_LE(filledCsm3_3[cell].value(), maxVal);
                    }

                }
                else {
                    EXPECT_TRUE(filledCsm3_5[cell].has_value());
                    EXPECT_TRUE(filledCsm6_5[cell].has_value());
                    EXPECT_TRUE(filledCsm3_3[cell].has_value());
                    EXPECT_EQ(defaultCsm[cell].value(), filledCsm3_5[cell].value());
                    EXPECT_EQ(defaultCsm[cell].value(), filledCsm6_5[cell].value());
                    EXPECT_EQ(defaultCsm[cell].value(), filledCsm3_3[cell].value());
                }
            }

            EXPECT_GE(filledHoles3_3, filledHoles3_5);
            EXPECT_GE(filledHoles6_5, filledHoles3_5);
            //3_3 and 6_5 are not comparable


            EXPECT_GE(filledHoles6_5, defaultHoles / 2);

            //symmetry test
            Raster<csm_t> reflectedDefault{ (Alignment)defaultCsm };
            for (rowcol_t row = 0; row < defaultCsm.nrow(); ++row) {
                for (rowcol_t col = 0; col < defaultCsm.ncol(); ++col) {
                    reflectedDefault.atRCUnsafe(row, col).has_value() = defaultCsm.atRCUnsafe(row, defaultCsm.ncol() - 1 - col).has_value();
                    if (defaultCsm.atRCUnsafe(row, defaultCsm.ncol() - 1 - col).has_value()) {
                        reflectedDefault.atRCUnsafe(row, col).value() = defaultCsm.atRCUnsafe(row, defaultCsm.ncol() - 1 - col).value();
                    }
                }
            }
            FillCsm algo{ 5, 6.0 };
            Raster<csm_t> reflectedFilled = algo.postProcess(reflectedDefault);
            for (rowcol_t row = 0; row < defaultCsm.nrow(); ++row) {
                for (rowcol_t col = 0; col < defaultCsm.ncol(); ++col) {
                    if (filledCsm6_5.atRCUnsafe(row, col).has_value()) {
                        EXPECT_TRUE(reflectedFilled.atRCUnsafe(row, defaultCsm.ncol() - 1 - col).has_value());
                        EXPECT_NEAR(filledCsm6_5.atRCUnsafe(row, col).value(), reflectedFilled.atRCUnsafe(row, defaultCsm.ncol() - 1 - col).value(), LAPIS_TEST_SMALL_EPSILON);
                    }
                    else {
                        EXPECT_FALSE(reflectedFilled.atRCUnsafe(row, defaultCsm.ncol() - 1 - col).has_value());
                    }
                }
            }
        }
    }
}