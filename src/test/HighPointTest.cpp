#include"HighPointTest.hpp"
#include"..\algorithms\HighPoints.hpp"

namespace lapis {
    const std::string highPointName = "HighPoints";
    const std::string highPointMinHeightName = "MinHeight";
    const std::string highPointMinDistName = "MinDist";


    VectorDataset<Point> applyHighPoint(const InputData& input, const TestCase& testCase)
    {
        Raster<csm_t> csm = applyDefaultCsmPostProcess(input);

        csm_t minHeight = testCase.params.at(highPointMinHeightName).as<csm_t>();
        coord_t minDist = testCase.params.at(highPointMinDistName).as<coord_t>();

        std::unique_ptr<UniqueIdGenerator> idGen = std::make_unique<GenerateIdByTile>(1, 0);
        std::vector<IDedTao> taos = HighPoints(minHeight, minDist).identifyTaos(csm, *idGen);

        VectorDataset<Point> outputDataset(csm.crs());
        outputDataset.addIntegerField("ID");
        for (const IDedTao& tao : taos) {
            coord_t x = csm.xFromCellUnsafe(tao.location);
            coord_t y = csm.yFromCellUnsafe(tao.location);
            Point p{ x,y,csm.crs() };
            outputDataset.addGeometry(p);
            outputDataset.back().setNumericField<taoid_t>("ID", tao.id);
        }

        return outputDataset;
    }

    TEST(TaoIDAlgoTest, HighPointTest) {
        for (const InputData& input : getInputDataList()) {
            for (const TestCase& testCase : getTests(highPointName)) {
                //there's a bunch of small annoying things in comparing to lidR's implementation of TAO identification, so I'm testing for invariants:
                //1. there is at least one tao identified, unless the raster has no cells above minheight
                //2. all taos are in different cells
                //3. all cells taos fall in have data and their value is at least minHeight
                //4. all taos are at least minDist away from each other
                //5. all tao cells are local maxima (or tied)

                Raster<csm_t> csm = applyDefaultCsmPostProcess(input);
                VectorDataset<Point> testDataset = applyHighPoint(input, testCase);
                csm_t minHeight = testCase.params.at(highPointMinHeightName).as<csm_t>();
                coord_t minDist = testCase.params.at(highPointMinDistName).as<coord_t>();

                if (testDataset.nFeature() == 0) {
                    bool hasValidCell = false;
                    for (cell_t cell : CellIterator(csm)) {
                        if (csm.atCellUnsafe(cell).has_value() && csm.atCellUnsafe(cell).value() >= minHeight) {
                            hasValidCell = true;
                            break;
                        }
                    }
                    ASSERT_FALSE(hasValidCell);
                    continue;
                }

                std::set<cell_t> occupiedCells;
                for (const auto& feature : testDataset) {
                    coord_t x = feature.getGeometry().x();
                    coord_t y = feature.getGeometry().y();
                    ASSERT_TRUE(csm.contains(x, y));
                    cell_t cell = csm.cellFromXYUnsafe(x, y);
                    ASSERT_TRUE(csm.atCellUnsafe(cell).has_value());
                    ASSERT_GE(csm.atCellUnsafe(cell).value(), minHeight);
                    ASSERT_TRUE(occupiedCells.find(cell) == occupiedCells.end());
                    occupiedCells.insert(cell);

                    rowcol_t row = csm.rowFromCellUnsafe(cell);
                    rowcol_t col = csm.colFromCellUnsafe(cell);
                    csm_t cellValue = csm.atCellUnsafe(cell).value();
                    for (rowcol_t rowBudge : {-1, 0, 1}) {
                        rowcol_t adjRow = row + rowBudge;
                        if (adjRow < 0 || adjRow >= csm.nrow()) {
                            continue;
                        }
                        for (rowcol_t colBudge : {-1, 0, 1}) {
                            rowcol_t adjCol = col + colBudge;
                            if (adjCol < 0 || adjCol >= csm.ncol()) {
                                continue;
                            }
                            if (adjRow == row && adjCol == col) {
                                continue;
                            }
                            cell_t adjCell = csm.cellFromRowColUnsafe(adjRow, adjCol);
                            if (csm.atCellUnsafe(adjCell).has_value()) {
                                ASSERT_LE(csm.atCellUnsafe(adjCell).value(), cellValue);
                            }
                        }
                    }
                }

                for (size_t i = 0; i < testDataset.nFeature(); ++i) {
                    coord_t x1 = testDataset.getFeature(i).getGeometry().x();
                    coord_t y1 = testDataset.getFeature(i).getGeometry().y();
                    for (size_t j = i + 1; j < testDataset.nFeature(); ++j) {
                        coord_t x2 = testDataset.getFeature(j).getGeometry().x();
                        coord_t y2 = testDataset.getFeature(j).getGeometry().y();
                        coord_t dist = std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
                        ASSERT_GE(dist, minDist);
                    }
                }
            }
        }
    }

}