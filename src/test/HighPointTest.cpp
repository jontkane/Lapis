#include"HighPointTest.hpp"
#include"..\algorithms\HighPoints.hpp"

namespace lapis {
    const std::string highPointName = "HighPoints";
    const std::string highPointMinHeightName = "MinHeight";
    const std::string highPointMinDistName = "MinDist";


    std::vector<IDedTao> applyHighPoint(const InputData& input, const TestCase& testCase)
    {
        Raster<csm_t> csm = applyDefaultCsmPostProcess(input);

        csm_t minHeight = testCase.params.at(highPointMinHeightName).as<csm_t>();
        coord_t minDist = testCase.params.at(highPointMinDistName).as<coord_t>();

        std::unique_ptr<UniqueIdGenerator> idGen = std::make_unique<GenerateIdByTile>(1, 0);
        return HighPoints(minHeight, minDist).identifyTaos(csm, *idGen);
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
                std::vector<IDedTao> testDataset = applyHighPoint(input, testCase);
                csm_t minHeight = testCase.params.at(highPointMinHeightName).as<csm_t>();
                coord_t minDist = testCase.params.at(highPointMinDistName).as<coord_t>();

                if (testDataset.size() == 0) {
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
                for (auto tao : testDataset) {
                    cell_t cell = tao.location;
                    ASSERT_TRUE(cell >= 0 && cell < csm.ncell());
                    coord_t x = csm.xFromCellUnsafe(cell);
                    coord_t y = csm.yFromCellUnsafe(cell);
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

                for (size_t i = 0; i < testDataset.size(); ++i) {
                    coord_t x1 = csm.xFromCellUnsafe(testDataset[i].location);
                    coord_t y1 = csm.yFromCellUnsafe(testDataset[i].location);
                    for (size_t j = i + 1; j < testDataset.size(); ++j) {
                        coord_t x2 = csm.xFromCellUnsafe(testDataset[j].location);
                        coord_t y2 = csm.yFromCellUnsafe(testDataset[j].location);
                        coord_t dist = std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
                        ASSERT_GE(dist, minDist);
                    }
                }
            }
        }
    }

}