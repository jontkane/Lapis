#include"WatershedSegmentTest.hpp"
#include"..\algorithms\WatershedSegment.hpp"

namespace lapis {

    const std::string watershedSegmentName = "WatershedSegment";
    const std::string watershedMinHeightName = "MinHeight";
    const std::string watershedMaxHeightName = "MaxHeight";
    const std::string watershedBinSizeName = "BinSize";
    const std::string watershedVectorizeName = "Vectorize";

    SegmentResults applyWatershedSegment(const InputData& input, const TestCase& testCase)
    {
        Raster<csm_t> defaultCsm = applyDefaultCsm(input);
        std::vector<IDedTao> taos = applyDefaultTreeIdentification(input);
        coord_t minHeight = testCase.params.at(watershedMinHeightName).as<coord_t>();
        coord_t maxHeight = testCase.params.at(watershedMaxHeightName).as<coord_t>();
        coord_t binSize = testCase.params.at(watershedBinSizeName).as<coord_t>();
        bool vectorize = testCase.params.at(watershedVectorizeName).as<bool>();
        WatershedSegment algo{ minHeight, maxHeight, binSize, vectorize };
        return algo.segment(defaultCsm, taos, defaultCsm);
    }

    TEST(TreeSegmentationTest, WatershedSegmentTest) {
        //we are checking for the following invariants:
        //1. the raster always exists
        //2. the unique values of the raster are exactly the same as the ids in the input tao list, excluding those whose cell is outside the minheight/maxheight range
        //3. every cell which is between minheight and maxheight in the csm has a value in the raster
        //4. the cell in the taolist is the maximum (or tied) within its segment
        //5. the vector exists if and only if vectorize is true
        //6. if the vector exists, the polygons contain exactly those cells whose values match the ID field of the polygon

        for (const InputData& input : getInputDataList()) {
            for (const TestCase& testCase : getTests(watershedSegmentName)) {
                Raster<csm_t> csm = applyDefaultCsm(input);
                std::vector<IDedTao> taos = applyDefaultTreeIdentification(input);
                coord_t minHeight = testCase.params.at(watershedMinHeightName).as<coord_t>();
                coord_t maxHeight = testCase.params.at(watershedMaxHeightName).as<coord_t>();
                bool vectorize = testCase.params.at(watershedVectorizeName).as<bool>();
                SegmentResults result = applyWatershedSegment(input, testCase);

                //1
                ASSERT_TRUE(result.raster.has_value());
                Raster<taoid_t>& segRaster = result.raster.value();
                ASSERT_EQ(segRaster.nrow(), csm.nrow());
                ASSERT_EQ(segRaster.ncol(), csm.ncol());
                
                std::unordered_map<taoid_t, std::vector<cell_t>> cellsById;
                for (cell_t cell : CellIterator(segRaster)) {
                    auto csmv = csm.atCellUnsafe(cell);
                    if (csmv.has_value() && csmv.value() >= minHeight && csmv.value() <= maxHeight) {
                        //3
                        ASSERT_TRUE(segRaster.atCellUnsafe(cell).has_value());
                        taoid_t id = segRaster.atCellUnsafe(cell).value();
                        cellsById[id].push_back(cell);
                    }
                }

                for (auto tao : taos) {
                    taoid_t id = tao.id;
                    cell_t taocell = tao.location;
                    csm_t taocsm = csm.atCellUnsafe(taocell).value();

                    //2
                    if (taocsm < minHeight || taocsm > maxHeight) {
                        EXPECT_TRUE(cellsById.find(id) == cellsById.end());
                        continue;
                    }
                    ASSERT_TRUE(cellsById.find(id) != cellsById.end());
                    const std::vector<cell_t>& cells = cellsById[id];

                    //4
                    csm_t maxInSegment = std::numeric_limits<csm_t>::lowest();
                    for (cell_t cell : cells) {
                        csm_t val = csm.atCellUnsafe(cell).value();
                        if (val > maxInSegment) {
                            maxInSegment = val;
                        }
                    }
                    csm_t taoVal = csm.atCellUnsafe(tao.location).value();
                    ASSERT_EQ(taoVal, maxInSegment);
                }

                //5
                EXPECT_EQ(vectorize, result.vector.has_value());

                if (!vectorize) {
                    continue;
                }
                VectorDataset<MultiPolygon>& vec = result.vector.value();
                for (const auto& feature : vec) {
                    taoid_t id = feature.getNumericField<taoid_t>("ID");

                    for (const auto& [segmentid, segmentcells] : cellsById) {
                        bool expectInside = (segmentid == id);
                        //6
                        for (cell_t cell : segmentcells) {
                            coord_t x = segRaster.xFromCellUnsafe(cell);
                            coord_t y = segRaster.yFromCellUnsafe(cell);
                            bool contains = feature.getGeometry().containsPoint(x, y);
                            EXPECT_EQ(contains, expectInside);
                        }
                    }
                    
                }
            }
        }
    }
}