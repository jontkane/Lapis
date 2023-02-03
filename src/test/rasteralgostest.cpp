#include"test_pch.hpp"
#include"..\gis\rasteralgos.hpp"

namespace lapis {

	class RasterAlgosTest : public ::testing::Test {
	public:

		Raster<double> r;
		Alignment a;

		void SetUp() override {
			a = Alignment(Extent(0, 4, 0, 4), 2, 2);
			r = Raster<double>(Alignment(Extent(0, 4, 0, 4), 4, 4));
			for (cell_t cell = 0; cell < r.ncell(); ++cell) {
				r[cell].value() = (int)cell;
				r[cell].has_value() = true;
			}
			r[0].has_value() = false;
			r[1].has_value() = false;
			r[4].has_value() = false;
			r[5].has_value() = false;
			r[15].has_value() = false;
		}
	};
	
	TEST_F(RasterAlgosTest, AggregateMax) {
		ViewFunc<double, double> f{ &viewMax<double, double> };
		auto out = aggregate(r, a, f);

		EXPECT_EQ((Alignment)out, a);
		
		EXPECT_FALSE(out[0].has_value());
		EXPECT_TRUE(out[1].has_value());
		EXPECT_TRUE(out[2].has_value());
		EXPECT_TRUE(out[3].has_value());

		EXPECT_EQ(out[1].value(), 7);
		EXPECT_EQ(out[2].value(), 13);
		EXPECT_EQ(out[3].value(), 14);
	}

	TEST_F(RasterAlgosTest, AggregateMean) {
		ViewFunc<double, double> f{ &viewMean<double, double> };
		auto out = aggregate(r, a, f);

		EXPECT_EQ((Alignment)out, a);

		EXPECT_FALSE(out[0].has_value());
		EXPECT_TRUE(out[1].has_value());
		EXPECT_TRUE(out[2].has_value());
		EXPECT_TRUE(out[3].has_value());

		EXPECT_NEAR(out[1].value(), 4.5, 0.01);
		EXPECT_NEAR(out[2].value(), 10.5, 0.01);
		EXPECT_NEAR(out[3].value(), 11.7, 0.1);
	}

	TEST_F(RasterAlgosTest, AggregateStdDev) {
		ViewFunc<double, double> f{ &viewStdDev<double, double> };
		auto out = aggregate(r, a, f);

		EXPECT_EQ((Alignment)out, a);

		EXPECT_FALSE(out[0].has_value());
		EXPECT_TRUE(out[1].has_value());
		EXPECT_TRUE(out[2].has_value());
		EXPECT_TRUE(out[3].has_value());

		EXPECT_NEAR(out[1].value(), 2.06, 0.01);
		EXPECT_NEAR(out[2].value(), 2.06, 0.01);
		EXPECT_NEAR(out[3].value(), 1.7, 0.01);
	}
}