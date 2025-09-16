#include"test_pch.hpp"
#include"TestUtils.hpp"

namespace lapis {

    const std::string alreadyNormalizedName = "AlreadyNormalized";
    const std::string minHeightParam = "MinHeight";
    const std::string maxHeightParam = "MaxHeight";

    
    std::vector<LasPoint> applyAlreadyNormalized(const InputData& input, const TestCase& testCase)
    {
        LasReader reader{ input.lazFile.string() };
        size_t nPoints = reader.nPoints();
        AlreadyNormalizedApplier applier{ std::move(reader),
            CoordRef(),
            testCase.params.at(minHeightParam).as<coord_t>(),
            testCase.params.at(maxHeightParam).as<coord_t>()
        };
        std::span<LasPoint> span = applier.getPoints(nPoints);
        std::vector<LasPoint> out{ span.begin(), span.end() };
        return out;
    }

    TEST(DemAlgorithms, AlreadyNormalizedTest) {

        //This algorithm is so simple that producing truth in R was kind of annoying
        //In particular, the fact that lidR refuses to write laz files with no points was a problem
        //So the test strategy is to check invariants, instead
        
        //We want to test that:
        //No points below MinHeight or above MaxHeight are returned
        //All other points are returned unchanged

        //Because it would be very annoying to do otherwise, we will assume that the order of the points is preserved
        //This is true in the current implementation, but is not technically guaranteed
        //If this ever changes, this test will need to be updated

        for (const InputData& input : getInputDataList()) {
            for (const TestCase& testCase : getTests(alreadyNormalizedName)) {

                std::vector<LasPoint> testPoints = applyAlreadyNormalized(input, testCase);
                LasReader reader{ input.lazFile.string() };
                LidarPointVector originalPoints = reader.getPoints(reader.nPoints());

                ASSERT_LE(testPoints.size(), originalPoints.size())
                    << "Additional points created in " << input.shortName;

                coord_t minHeight = testCase.params.at(minHeightParam).as<coord_t>();
                coord_t maxHeight = testCase.params.at(maxHeightParam).as<coord_t>();


                auto testIt = testPoints.begin();
                auto originalIt = originalPoints.begin();
                while (testIt != testPoints.end() && originalIt != originalPoints.end()) {
                    while ((originalIt->z < minHeight ||
                        originalIt->z > maxHeight)
                        && originalIt != originalPoints.end()
                        ) {
                        ++originalIt;
                    }
                    if (originalIt == originalPoints.end()) {
                        ASSERT_TRUE(false) << "Reached the end of original points before finding a valid point in "
                            << input.shortName;
                    }
                    if (testIt->z < minHeight ||
                        testIt->z > maxHeight) {
                        ASSERT_TRUE(false) << "Point outside height range in " << input.shortName;
                    } else {
                        ASSERT_NEAR(testIt->x, originalIt->x, LAPIS_TEST_SMALL_EPSILON)
                            << "X coordinate mismatch in " << input.shortName;
                        ASSERT_NEAR(testIt->y, originalIt->y, LAPIS_TEST_SMALL_EPSILON)
                            << "Y coordinate mismatch in " << input.shortName;
                        ASSERT_NEAR(testIt->z, originalIt->z, LAPIS_TEST_SMALL_EPSILON)
                            << "Z coordinate mismatch in " << input.shortName;
                    }
                    ++testIt;
                    ++originalIt;
                }
            }
        }
    }
}