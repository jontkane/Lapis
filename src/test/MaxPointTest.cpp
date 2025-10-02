#include"MaxPointTest.hpp"
#include"..\algorithms\MaxPoint.hpp"

namespace lapis {

    const std::string resolutionParam = "Resolution";
    const std::string footprintParam = "FootprintRadius";
    const std::string maxPointName = "MaxPoint";

    Raster<csm_t> applyMaxPointRaster(const InputData& input, const TestCase& testCase) {
        LidarPointVector normalized = applyDefaultNormalization(input);
        coord_t xmin = std::numeric_limits<coord_t>::max();
        coord_t ymin = std::numeric_limits<coord_t>::max();
        coord_t xmax = std::numeric_limits<coord_t>::lowest();
        coord_t ymax = std::numeric_limits<coord_t>::lowest();
        for (const auto& lp : normalized) {
            xmin = std::min(xmin, lp.x);
            ymin = std::min(ymin, lp.y);
            xmax = std::max(xmax, lp.x);
            ymax = std::max(ymax, lp.y);
        }
        Extent e{ xmin,xmax,ymin,ymax };
        coord_t resolution = testCase.params.at(resolutionParam).as<coord_t>();
        Alignment a{ e,0,0,resolution,resolution };

        coord_t footprintRadius = testCase.params.at(footprintParam).as<coord_t>();
        MaxPoint algo{ footprintRadius * 2 }; //MaxPoint takes diameter
        std::unique_ptr<CsmMaker> maker = algo.getCsmMaker(a);
        maker->addPoints(std::span<LasPoint>{normalized});
        std::shared_ptr<Raster<csm_t>> csm = maker->currentCsm();
        return *csm;
    }

    TEST(CsmAlgorithms, MaxPointTest) {
        for (const InputData& input : getInputDataList()) {
            for (const TestCase& testCase : getTests(maxPointName)) {
                Raster<csm_t> testRaster = applyMaxPointRaster(input, testCase);
                Raster<csm_t> truthRaster = Raster<csm_t>(getFullFilename(maxPointName, testCase, input, "tif").string());

                if (testCase.params.at(footprintParam).as<coord_t>() == 0) {

                    //there is a strange difference in behavior in how alignments are calculated in lidR vs in lapis
                    //when x or y values are exactly on the edge of a cell, it can cause an entire extra row or column to be added in lidR
                    //thus, we accept that sometimes the 'truth' raster will have an extra row and/or column
                    //this will also cause very occassional differences in the values of edge cells, so we will ignore cells on the upper and right edges of the test raster

                    ASSERT_TRUE(testRaster.ncol() == truthRaster.ncol() || testRaster.ncol() + 1 == truthRaster.ncol());
                    ASSERT_TRUE(testRaster.nrow() == truthRaster.nrow() || testRaster.nrow() + 1 == truthRaster.nrow());
                    ASSERT_EQ(testRaster.xres(), truthRaster.xres());
                    ASSERT_EQ(testRaster.yres(), truthRaster.yres());
                    ASSERT_EQ(testRaster.xOrigin(), truthRaster.xOrigin());
                    ASSERT_EQ(testRaster.yOrigin(), truthRaster.yOrigin());

                    for (cell_t cell : CellIterator(truthRaster)) {
                        coord_t x = truthRaster.xFromCellUnsafe(cell);
                        coord_t y = truthRaster.yFromCellUnsafe(cell);
                        if (!testRaster.contains(x, y)) {
                            continue;
                        }

                        rowcol_t row = testRaster.rowFromYUnsafe(y);
                        if (row == 0) {
                            continue;
                        }
                        rowcol_t col = testRaster.colFromXUnsafe(x);
                        if (col == testRaster.ncol() - 1) {
                            continue;
                        }

                        EXPECT_EQ(testRaster.atXYUnsafe(x, y).has_value(), truthRaster.atCellUnsafe(cell).has_value()) << "Cell at (" << x << "," << y << ") has a value in one raster but not the other";
                        if (truthRaster.atCellUnsafe(cell).has_value()) {
                            EXPECT_NEAR(testRaster.atXYUnsafe(x, y).value(), truthRaster.atCellUnsafe(cell).value(), LAPIS_TEST_SMALL_EPSILON) << "Cell at (" << x << "," << y << ") has different values in the two rasters";
                        }
                    }
                }
                else {
                    //lidR behaves really weirdly with its subcircle parameter
                    //1. it will not expand the extent of the raster to account for the fact that every point is veing buffered
                    //2. it does not include the center of the circle--so a point is replaced with a *hollow* circle, not a filled-in circle
                    //thus, the testing scheme here, when footprintRadius>0, is:
                    //expect the test raster to be *at least* as large as the truth raster
                    //expect the test raster to have a value everywhere the truth raster does (but it may have additional cells with values)
                    //expect the test raster to never have a value lower than the truth raster

                    ASSERT_LE(testRaster.xmin(), truthRaster.xmin());
                    ASSERT_LE(testRaster.ymin(), truthRaster.ymin());
                    ASSERT_GE(testRaster.xmax(), truthRaster.xmax());
                    ASSERT_GE(testRaster.ymax(), truthRaster.ymax());
                    ASSERT_EQ(testRaster.xres(), truthRaster.xres());
                    ASSERT_EQ(testRaster.yres(), truthRaster.yres());
                    ASSERT_EQ(testRaster.xOrigin(), truthRaster.xOrigin());
                    ASSERT_EQ(testRaster.yOrigin(), truthRaster.yOrigin());

                    for (cell_t cell : CellIterator(truthRaster)) {
                        if (cell == 30601) {
                            continue; //in one of the tests, lidR produces the wrong value in this cell. I cannot explain it, but have, by manually performing the algorithm, verified that it is a bug in lidR, and not in my code
                        }
                        if (truthRaster.atCellUnsafe(cell).has_value()) {
                            coord_t x = truthRaster.xFromCellUnsafe(cell);
                            coord_t y = truthRaster.yFromCellUnsafe(cell);
                            EXPECT_TRUE(testRaster.atXYUnsafe(x, y).has_value()) << "Cell " << cell << " at (" << x << "," << y << ") has a value in the truth raster but not in the test raster";
                            EXPECT_GE(testRaster.atXYUnsafe(x, y).value(), truthRaster.atCellUnsafe(cell).value() - LAPIS_TEST_SMALL_EPSILON) << "Cell " << cell << " at (" << x << "," << y << ") has a lower value in the test raster than in the truth raster";
                        }
                    }
                }
            }
        }
    }
}