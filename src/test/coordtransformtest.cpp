#include"..\gis\CoordTransform.hpp"
#include<gtest/gtest.h>

namespace lapis {

	class CoordTransformTest : public ::testing::Test {
	public:
		struct xyz {
			coord_t x, y, z;
		};
		inline static CoordRef empty, stateplane, utm;
		std::vector<xyz> v;

		static void SetUpTestSuite() {
			empty = CoordRef{ "" };
			stateplane = CoordRef{ "2927" };
			utm = CoordRef{ "32611" };
		}

		void SetUp() override {
			v = { {0,0,0},
				{1000,1000,1000},
				{5000,5000,5000} };
		}
	};

	void closeXYZ(const std::vector<CoordTransformTest::xyz>& actual, const std::vector<CoordTransformTest::xyz>& expected) {
		for (int i = 0; i < actual.size(); ++i) {
			EXPECT_NEAR(actual[i].x, expected[i].x, 1);
			EXPECT_NEAR(actual[i].y, expected[i].y, 1);
			EXPECT_NEAR(actual[i].z, expected[i].z, 1);
		}
	}

	TEST_F(CoordTransformTest, noOp) {
		std::vector<CoordTransform> noops = {
			CoordTransform(empty,empty)
			,CoordTransform(empty, stateplane)
			,CoordTransform(utm, empty)
			,CoordTransform(utm,utm)
		};
		std::vector<xyz> exp = { {0,0,0},
				{1000,1000,1000},
				{5000,5000,5000} };
		for (int i = 0; i < noops.size(); ++i) {
			noops[i].transformXY(v);
			closeXYZ(v, exp);
			noops[i].transformXYZ(v);
			closeXYZ(v, exp);
		}
	}

	TEST_F(CoordTransformTest, xyOnly) {
		CoordTransform tr{ stateplane,utm };
		tr.transformXY(v, 1);
		std::vector<xyz> exp = {
			{0,0,0},
			{-275004,5047656,1000},
			{-273726,5048831,5000}
		};
		closeXYZ(v, exp);
	}

	TEST_F(CoordTransformTest, xyzTr) {
		CoordTransform tr{ stateplane,utm };
		tr.transformXYZ(v, 1);
		std::vector<xyz> exp = {
			{0,0,0},
			{-275004,5047656,304},
			{-273726,5048831,1524}
		};
		closeXYZ(v, exp);
	}
		
}