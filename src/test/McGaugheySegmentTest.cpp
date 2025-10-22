#include"McGaugheySegmentTest.hpp"
#include"..\algorithms\McGaugheySegment.hpp"

namespace lapis {

    const std::string mcgaugheySegmentName = "McGaugheySegment";
    const std::string mcgaugheyNVerticesName = "NVertices";
    const std::string mcgaugheySlopeChangeMultiplierName = "SlopeChangeMultiplier";
    const std::string mcgaugheyHeightCutoffMultiplierName = "HeightCutoffMultiplier";
    const std::string mcgaugheyMaxDistMultiplierName = "MaxDistMultiplier";
    const std::string mcgaugheySmoothTypeName = "SmoothType";

    SegmentResults applyMcGaugheySegment(const InputData& input, const TestCase& testCase)
    {
        Raster<csm_t> defaultCsm = applyDefaultCsmPostProcess(input);
        std::vector<IDedTao> taos = applyDefaultTreeIdentification(input);
        int nVertices = testCase.params.at(mcgaugheyNVerticesName).as<int>();
        csm_t slopeChangeMultiplier = testCase.params.at(mcgaugheySlopeChangeMultiplierName).as<csm_t>();
        csm_t heightCutoffMultiplier = testCase.params.at(mcgaugheyHeightCutoffMultiplierName).as<csm_t>();
        coord_t maxDistMultiplier = testCase.params.at(mcgaugheyMaxDistMultiplierName).as<coord_t>();
        std::string smoothTypeStr = testCase.params.at(mcgaugheySmoothTypeName).as<std::string>();
        McGaugheySmoothType smoothType;
        if (smoothTypeStr == "fusion") {
            smoothType = McGaugheySmoothType::fusion;
        }
        else if (smoothTypeStr == "simple") {
            smoothType = McGaugheySmoothType::simple;
        }
        else {
            smoothType = McGaugheySmoothType::none;
        }

        McGaugheySegment algo{ nVertices, slopeChangeMultiplier, heightCutoffMultiplier, maxDistMultiplier, smoothType };
        return algo.segment(defaultCsm, taos, defaultCsm);
    }

    TEST(TreeSegmentationTest, McGaugheySegmentTest) {

        //this is extremely difficult to test meaningfully, because the only truth is the output from a program which has these parameters instead of customizable
        //As usual in difficult cases, we are just testing some invariants:
        //1. the vector always exists, and the raster never exists
        //2. each tao in the input has a corresponding polygon in the output. It will be contained in that polygon, and the ID field will match that tao's id
        //3. each multipolygon has exactly one polygon, with no inner rings, and an outer ring with exactly nvertices vertices

        //this unfortunately leaves a lot untested

        for (const InputData& input : getInputDataList()) {
            for (const TestCase& testCase : getTests(mcgaugheySegmentName)) {
                SegmentResults result = applyMcGaugheySegment(input, testCase);

                //1
                ASSERT_TRUE(result.vector.has_value());
                ASSERT_FALSE(result.raster.has_value());

                Raster<csm_t> csm = applyDefaultCsmPostProcess(input);
                std::vector<IDedTao> taos = applyDefaultTreeIdentification(input);
                VectorDataset<MultiPolygon>& segVector = result.vector.value();
                ASSERT_EQ(segVector.nFeature(), taos.size());
                int nVertices = testCase.params.at(mcgaugheyNVerticesName).as<int>();

                std::unordered_map<taoid_t, cell_t> taosAsMap;
                for (auto tao : taos) {
                    taosAsMap[tao.id] = tao.location;
                }
                for (size_t i = 0; i < segVector.nFeature(); i++) {
                    ConstFeature<MultiPolygon> feature = segVector.getFeature(i);
                    taoid_t id = feature.getNumericField<taoid_t>("ID");
                    auto taoIt = taosAsMap.find(id);
                    ASSERT_NE(taoIt, taosAsMap.end()); //2a
                    cell_t taoCell = taoIt->second;
                    coord_t taoX = csm.xFromCell(taoCell);
                    coord_t taoY = csm.yFromCell(taoCell);
                    const MultiPolygon& multipolygon = feature.getGeometry();

                    //containsPoint only tests strict containment, so we have to manually test for the edge
                    //fortunately, if the algorithm is working correctly, the only way that can happen is if the tao is exactly on a vertex
                    bool inside = multipolygon.containsPoint(taoX, taoY);
                    if (!inside) {
                        for (const Polygon& polygon : multipolygon) {
                            for (const CoordXY& vertex : polygon.getOuterRing()) {
                                if (vertex.x == taoX && vertex.y == taoY) {
                                    inside = true;
                                    break;
                                }
                            }
                            if (inside) {
                                break;
                            }
                        }
                    }
                    ASSERT_TRUE(inside); //2b

                    //3
                    ASSERT_EQ(multipolygon.nPolygon(), 1);
                    const Polygon& polygon = *multipolygon.begin();
                    ASSERT_EQ(polygon.nInnerRings(), 0);
                    //remember that the polygon's outer ring repeats the first vertex at the end
                    ASSERT_EQ(polygon.getOuterRing().size(), nVertices+1);
                }
            }
        }
    }
}