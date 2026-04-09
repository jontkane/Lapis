#include"VendorRasterTest.hpp"

namespace lapis {

    const std::string vendorRasterName = "VendorRaster";
    const std::string minHeightParam = "MinHeight";
    const std::string maxHeightParam = "MaxHeight";

    class FileGetterForTest {
    public:
        FileGetterForTest(const InputData& input) {
            for (const auto& tifFile : input.tifFile) {
                Alignment align{ tifFile.string() };
                _aligns.push_back(align);
                Raster<coord_t> raster{ tifFile.string() };
                _rasters.push_back(std::move(raster));
            }
        }
        const std::vector<Alignment>& demAligns() const {
            return _aligns;
        }
        Alignment demAlign(size_t n, const CoordRef& crs) const {
            return transformAlignment(_aligns[n], crs);
        }
        std::optional<Raster<coord_t>> getDem(size_t n, const Extent& e) {

            if (n >= _rasters.size()) {
                return std::optional<Raster<coord_t>>();
            }

            Alignment& thisAlign = _aligns[n];

            Extent projE = QuadExtent(e, thisAlign.crs()).outerExtent();
            if (!projE.overlaps(thisAlign)) {
                return std::optional<Raster<coord_t>>();
            }

            projE.defineCRS(CoordRef("")); //if there's a crs override, then there may be a spurious CRS mismatch

            std::optional<Raster<coord_t>> outopt = cropRaster(
                _rasters[n],
                projE,
                SnapType::out
            );
            return outopt;
        }
        size_t nDem() const {
            return _rasters.size();
        }
    private:
        //this is horribly inefficient if test data ever becomes large
        std::vector<Alignment> _aligns;
        std::vector<Raster<coord_t>> _rasters;
    };

    std::vector<LasPoint> applyVendorRaster(const InputData& input, const TestCase& testCase) {
        LasReader reader{ input.lazFile.string() };
        size_t nPoints = reader.nPoints();
        FileGetterForTest getter{ input };
        VendorRasterApplier<FileGetterForTest> applier{ &getter, std::move(reader),
            reader.crs(),
            testCase.params.at(minHeightParam).as<coord_t>(),
            testCase.params.at(maxHeightParam).as<coord_t>()
        };
        std::span<LasPoint> span = applier.getPoints(nPoints);
        std::vector<LasPoint> out{ span.begin(), span.end() };
        return out;
    }
    TEST(DemAlgorithms, VendorRasterTest) {
        for (const InputData& input : getInputDataList()) {
            for (const TestCase& testCase : getTests(vendorRasterName)) {

                std::vector<LasPoint> testPoints = applyVendorRaster(input, testCase);
                LasReader truth{ getFullFilename(vendorRasterName, testCase, input, "laz").string()};
                LidarPointVector truthPoints = truth.getPoints(truth.nPoints());

                //We can generally expect a few decimeters of difference in the Z value between lapis and lidR
                //This causes issues when the Z value is within [minheight, maxheight] for one but not the other
                //So we can't assume that points with the same index correspond

                ASSERT_NEAR((double)testPoints.size(), (double)truthPoints.size(), testPoints.size()*0.1)
                    << "Number of points does not match for " << input.shortName;

                class CoordXYHasher {
                    public:
                    std::size_t operator()(const CoordXY& coord) const {
                        return std::hash<coord_t>()(coord.x) ^ (std::hash<coord_t>()(coord.y) << 1);
                    }
                };
                class CoordXYComparator {
                    public:
                    bool operator()(const CoordXY& lhs, const CoordXY& rhs) const {
                        return lhs.x == rhs.x && lhs.y == rhs.y;
                    }
                };;

                std::unordered_map<CoordXY, coord_t, CoordXYHasher, CoordXYComparator> testMap;
                std::unordered_map<CoordXY, coord_t, CoordXYHasher, CoordXYComparator> truthMap;

                //the whole testing strategy doesn't work if two points have the same x/y, so we remove all duplicates
                std::unordered_set<CoordXY, CoordXYHasher, CoordXYComparator> toRemove;
                
                for (size_t i = 0; i < testPoints.size(); ++i) {
                    CoordXY coord(testPoints[i].x, testPoints[i].y);
                    if (testMap.contains(coord)) {
                        toRemove.insert(coord);
                    }
                    testMap[coord] = testPoints[i].z;
                }
                for (size_t i = 0; i < truthPoints.size(); ++i) {
                    CoordXY coord(truthPoints[i].x, truthPoints[i].y);
                    if (truthMap.contains(coord)) {
                        toRemove.insert(coord);
                    }
                    truthMap[coord] = truthPoints[i].z;
                }
                for (const auto& coord : toRemove) {
                    testMap.erase(coord);
                    truthMap.erase(coord);
                }

                size_t matchedPoints = 0;
                size_t outsideSmallEpsilon = 0;
                for (const auto& [coord, testZ] : testMap) {
                    auto it = truthMap.find(coord);
                    if (it != truthMap.end()) {
                        ++matchedPoints;
                        ASSERT_NEAR(testZ, it->second, LAPIS_TEST_LARGE_EPSILON)
                            << "Z value mismatch for point at (" << coord.x << ", " << coord.y << ") in " << input.shortName;
                        if (std::abs(testZ - it->second) > LAPIS_TEST_SMALL_EPSILON) {
                            ++outsideSmallEpsilon;
                        }
                    }
                }
                ASSERT_NEAR((double)matchedPoints, (double)truthPoints.size(), truthPoints.size() * 0.1)
                    << "Large number of unmatched points for " << input.shortName;
                ASSERT_LE(outsideSmallEpsilon, matchedPoints * 0.01)
                    << "Large number of points with large Z value differences for " << input.shortName;
            }
        }
    }
}