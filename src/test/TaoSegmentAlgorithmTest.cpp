#include"test_pch.hpp"
#include<unordered_set>
#include"..\algorithms\AllTaoSegmentAlgorithms.hpp"


namespace lapis {

	TEST(TaoSegmentAlgorithmTest, WastershedTest) {
		Raster<csm_t> csm(Alignment(Extent(0, 3, 0, 3), 3, 3));
		std::vector<csm_t> values = {
			7,6,5,
			3,4,3,
			9,1,2,
		};
		std::vector<cell_t> highPoints = { 0,6 };

		for (cell_t cell = 0; cell < csm.ncell(); ++cell) {
			csm[cell].has_value() = true;
			csm[cell].value() = values[cell];
		}

		std::unordered_set<cell_t> segmentOne = { 0,1,2,5 };
		std::unordered_set<cell_t> segmentTwo = { 3,4,6,8 };
		std::unordered_set<cell_t> na = { 7 };


		struct Successive : public UniqueIdGenerator {
		public:
			taoid_t previd = 0;
			taoid_t nextId() override {
				previd++;
				return(previd);
			}
		};
		Successive idGen;

		WatershedSegment algo{ 1.1,10,0.1 };
		Raster<taoid_t> segments = algo.segment(csm, highPoints, idGen);
		ASSERT_TRUE(segments.isSameAlignment(csm));

		taoid_t segOneValue = 1000;
		taoid_t segTwoValue = 1000;

		for (cell_t cell = 0; cell < segments.ncell(); ++cell) {
			if (na.contains(cell)) {
				EXPECT_TRUE(segments[cell].has_value());
				EXPECT_EQ(segments[cell].value(), 0);
				continue;
			}

			EXPECT_TRUE(segments[cell].has_value());
			if (segmentOne.contains(cell)) {
				if (segOneValue > 10) {
					segOneValue = segments[cell].value();
				}
				else {
					EXPECT_EQ(segOneValue, segments[cell].value());
				}
			}
			if (segmentTwo.contains(cell)) {
				if (segTwoValue > 10) {
					segTwoValue = segments[cell].value();
				}
				else {
					EXPECT_EQ(segTwoValue, segments[cell].value());
				}
			}
		}
	}
}