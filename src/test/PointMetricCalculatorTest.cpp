#include"..\PointMetricCalculator.hpp"
#include<gtest/gtest.h>

namespace lapis {

	class PointMetricCalculatorTest : public ::testing::Test {
	public:
		static void SetUpTestSuite() {
			PointMetricCalculator::setInfo(2, 100, 0.1);
		}

		PointMetricCalculator sparsePMC;
		PointMetricCalculator densePMC;
		Raster<metric_t> r;

		void SetUp() override {
			r = Raster<metric_t>(Alignment(Extent(0, 1, 0, 2), 1, 2));
			for (coord_t i = -10; i < 21; ++i) {
				sparsePMC.addPoint({ 0,0,i,0 });
			}

			for (int i = 0; i < 1000; ++i) {
				densePMC.addPoint({ 0,0,2 + i * 0.01,0 });
			}
		}

		void TearDown() override {
			sparsePMC.cleanUp();
		}
	};

	TEST_F(PointMetricCalculatorTest, meanCanopy) {
		sparsePMC.meanCanopy(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_NEAR(r[0].value(), 11, 0.1);
		EXPECT_FALSE(r[1].has_value());

		densePMC.meanCanopy(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_NEAR(r[0].value(), 6.995, 0.1);
		EXPECT_FALSE(r[1].has_value());
	}

	TEST_F(PointMetricCalculatorTest, stdDevCanopy) {
		sparsePMC.stdDevCanopy(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_NEAR(r[0].value(), 5.477, 0.1);
		EXPECT_FALSE(r[1].has_value());

		densePMC.stdDevCanopy(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_NEAR(r[0].value(), 2.887, 0.1);
		EXPECT_FALSE(r[1].has_value());
	}

	TEST_F(PointMetricCalculatorTest, returnCount) {
		sparsePMC.returnCount(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_EQ(r[0].value(), 31);
		EXPECT_FALSE(r[1].has_value());

		densePMC.returnCount(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_EQ(r[0].value(), 1000);
		EXPECT_FALSE(r[1].has_value());
	}

	TEST_F(PointMetricCalculatorTest, canopyCover) {
		sparsePMC.canopyCover(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_NEAR(r[0].value(),61.2, 0.1);
		EXPECT_FALSE(r[1].has_value());

		densePMC.canopyCover(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_EQ(r[0].value(), 100);
		EXPECT_FALSE(r[1].has_value());
	}

	TEST_F(PointMetricCalculatorTest, p25Canopy) {
		sparsePMC.p25Canopy(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_NEAR(r[0].value(), 6.5, 0.1);
		EXPECT_FALSE(r[1].has_value());

		densePMC.p25Canopy(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_NEAR(r[0].value(), 4.4975, 0.1);
		EXPECT_FALSE(r[1].has_value());
	}

	TEST_F(PointMetricCalculatorTest, p50Canopy) {
		sparsePMC.p50Canopy(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_NEAR(r[0].value(), 11, 0.1);
		EXPECT_FALSE(r[1].has_value());

		densePMC.p50Canopy(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_NEAR(r[0].value(), 6.995, 0.1);
		EXPECT_FALSE(r[1].has_value());
	}

	TEST_F(PointMetricCalculatorTest, p75Canopy) {
		sparsePMC.p75Canopy(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_NEAR(r[0].value(), 15.5, 0.1);
		EXPECT_FALSE(r[1].has_value());

		densePMC.p75Canopy(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_NEAR(r[0].value(), 9.4925, 0.1);
		EXPECT_FALSE(r[1].has_value());
	}

	TEST_F(PointMetricCalculatorTest, p95Canopy) {
		sparsePMC.p95Canopy(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_NEAR(r[0].value(), 19.1, 0.1);
		EXPECT_FALSE(r[1].has_value());

		densePMC.p95Canopy(r, 0);
		EXPECT_TRUE(r[0].has_value());
		EXPECT_NEAR(r[0].value(), 11.4905, 0.1);
		EXPECT_FALSE(r[1].has_value());
	}
	
}