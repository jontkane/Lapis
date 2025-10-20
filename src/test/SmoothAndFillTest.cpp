#include"SmoothAndFillTest.hpp"
#include"..\algorithms\SmoothAndFill.hpp"

namespace lapis {
    const std::string smoothAndFillName = "SmoothAndFill";
    const std::string smoothWindowParamName = "SmoothWindow";
    const std::string fillCsmNeighborsNeededParam = "NeighborsNeeded";
    const std::string fillCsmLookDistParam = "LookDist";


    Raster<csm_t> applySmoothAndFill(const InputData& input, const TestCase& testCase)
    {
        Raster<csm_t> defaultCsm = applyDefaultCsm(input);
        int smoothWindow = testCase.params.at(smoothWindowParamName).as<int>();
        coord_t lookDist = testCase.params.at(fillCsmLookDistParam).as<coord_t>();
        int neighborsNeeded = testCase.params.at(fillCsmNeighborsNeededParam).as<int>();
        SmoothAndFill algo{ smoothWindow, neighborsNeeded, lookDist };
        return algo.postProcess(defaultCsm);
    }

    TEST(CsmPostProcessorTests, SmoothAndFillTest) {
        //because the fill algorithm lapis uses isn't from anything else, we can't directly compare against truth
        //however, smooth and fill is conceptually very simple:
        //in cells where the original data is present, the output should be equivalent to smoothing
        //in vells where the original data is missing, the output should be equivalent to filling

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
            std::vector<int> smoothWindows = { 3,5 };
            std::vector<int> neighborsNeededList = { 3,5 };
            std::vector<coord_t> lookDists = { 3.0,6.0 };
            for (int smoothWindow : smoothWindows) {
                for (int neighborsNeeded : neighborsNeededList) {
                    for (coord_t lookDist : lookDists) {
                        FillCsm fillAlgo{ neighborsNeeded, lookDist };
                        SmoothCsm smoothAlgo{ smoothWindow };
                        SmoothAndFill algo{ smoothWindow, neighborsNeeded, lookDist };
                        Raster<csm_t> defaultCsm = applyDefaultCsm(input);
                        Raster<csm_t> smoothedCsm = smoothAlgo.postProcess(defaultCsm);
                        Raster<csm_t> filledCsm = fillAlgo.postProcess(defaultCsm);
                        Raster<csm_t> smoothAndFillCsm = algo.postProcess(defaultCsm);

                        checkSameAlignment(defaultCsm, smoothAndFillCsm);
                        checkSameAlignment(smoothAndFillCsm, smoothedCsm);
                        checkSameAlignment(smoothAndFillCsm, filledCsm);

                        for (cell_t cell : CellIterator(smoothAndFillCsm)) {
                            if (defaultCsm[cell].has_value()) {
                                //should be smoothed value
                                EXPECT_TRUE(smoothedCsm[cell].has_value());
                                EXPECT_TRUE(smoothAndFillCsm[cell].has_value());
                                EXPECT_NEAR(smoothAndFillCsm[cell].value(), smoothedCsm[cell].value(), LAPIS_TEST_SMALL_EPSILON);
                            }
                            else {
                                EXPECT_EQ(smoothAndFillCsm[cell].has_value(), filledCsm[cell].has_value());
                                if (filledCsm[cell].has_value()) {
                                    EXPECT_NEAR(smoothAndFillCsm[cell].value(), filledCsm[cell].value(), LAPIS_TEST_SMALL_EPSILON);
                                }
                            }
                        }

                    }
                }
            }
        }
    }

}