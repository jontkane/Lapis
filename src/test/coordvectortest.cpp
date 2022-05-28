#include"..\gis\CoordVector.hpp"
#include<gtest/gtest.h>

namespace lapis {
	class CoordVectorTest : public ::testing::Test {
	public:
		inline static CoordRef empty, stateplane, utm;
		CoordXYZVector v;

		static void SetUpTestSuite() {
			empty = CoordRef{ "" };
			stateplane = CoordRef{ "2927" };
			utm = CoordRef{ "32611" };
		}

		void SetUp() override {
			v.crs = utm;
			v.push_back({ 0,0,0 });
		}
	};

	void closeCoordVector(const CoordXYZVector& actual, const CoordXYZVector& expected) {
		EXPECT_TRUE(actual.crs.isConsistent(expected.crs));
		EXPECT_TRUE(actual.crs.isEmpty() == expected.crs.isEmpty());
		EXPECT_EQ(actual.size(), expected.size());
		for (int i = 0; i < actual.size(); ++i) {
			EXPECT_NEAR(actual[i].x, expected[i].x, 1);
			EXPECT_NEAR(actual[i].y, expected[i].y, 1);
			EXPECT_NEAR(actual[i].z, expected[i].z, 1);
		}
	}

	TEST_F(CoordVectorTest, addPoints) {
		CoordXYZVector emptyCRSVector;
		emptyCRSVector.push_back({ 500,500,500 });
		CoordXYZVector stateplaneVector{stateplane};
		stateplaneVector.push_back({ 1000,1000,1000 });
		CoordXYZVector utmVector{utm};
		utmVector.push_back({ 5000,5000,5000 });



		CoordXYZVector expected;
		expected.crs = utm;
		expected.push_back({ 0,0,0 });
		expected.push_back({ 500,500,500 });
		expected.push_back({ -275004,5047656,304 });
		expected.push_back({ 5000,5000,5000 });

		v.addAll(emptyCRSVector);
		v.addAll(stateplaneVector);
		v.addAll(utmVector);

		closeCoordVector(v, expected);
	}

	TEST_F(CoordVectorTest, transform) {
		v.push_back({ 1000,1000,1000 });
		v.push_back({ 5000,5000,5000 });
		v.transform(stateplane);

		CoordXYZVector expected{ stateplane };
		expected.push_back({ 1156780,-18291488,0 });
		expected.push_back({ 1161216,-18287161,3280 });
		expected.push_back({ 1178952,-18269851,16404 });

		closeCoordVector(v, expected);
	}
}