#include<gtest/gtest.h>
#include"..\csmalgos.hpp"

namespace lapis {
	TEST(CsmAlgosTest, smoothAndFillTest) {
		Raster<csm_t> r{ Alignment(Extent(0,3,0,3),3,3) };
		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			r[cell].value() = cell;
			r[cell].has_value() = true;
		}
		r[4].has_value() = false;
		r[6].has_value() = false;

		auto test = smoothAndFill(r, 3, 6, std::vector<cell_t>());
		std::vector<csm_t> expected = {
			(0.+1.+3.)/3.,
			(0.+1.+2.+3.+5.)/5.,
			(1.+2.+5.)/3.,
			(0.+1.+3.+7.)/4.,
			(0.+1.+2.+3.+5.+7.+8.)/7,
			(1.+2.+5.+7.+8.)/5.,
			NAN,
			(3.+5.+7.+8.)/4.,
			(5.+7.+8.)/3.
		};
		std::vector<bool> expectedHasValue = {
			true,
			true,
			true,
			true,
			true,
			true,
			false,
			true,
			true
		};
		for (int i = 0; i < r.ncell(); ++i) {
			if (expectedHasValue[i]) {
				EXPECT_TRUE(test[i].has_value()) << "on cell " + std::to_string(i);
				EXPECT_NEAR(test[i].value(), expected[i], 0.0001) << "on cell " + std::to_string(i);
			}
			else {
				EXPECT_FALSE(test[i].has_value()) << "on cell " + std::to_string(i);
			}
		}

		test = smoothAndFill(r, 3, 5, { 1 });
		expected[1] = r[1].value();
		for (int i = 0; i < r.ncell(); ++i) {
			if (expectedHasValue[i]) {
				EXPECT_TRUE(test[i].has_value()) << "on cell " + std::to_string(i);
				EXPECT_NEAR(test[i].value(), expected[i], 0.0001) << "on cell " + std::to_string(i);
			}
			else {
				EXPECT_FALSE(test[i].has_value()) << "on cell " + std::to_string(i);
			}
		}
	}

	TEST(CsmAlgosTest, identifyHighPointsTest) {
		Raster<csm_t> r{ Alignment(Extent(0,3,0,3),3,3) };
		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			rowcol_t row = r.rowFromCellUnsafe(cell);
			rowcol_t col = r.colFromCellUnsafe(cell);
			if (row == 1 || col == 1) {
				r[cell].value() = 1;
			}
			else {
				r[cell].value() = 10;
			}
			r[cell].has_value() = true;
		}
		r[0].has_value() = false;
		r[2].value() = 5;
		r[3].has_value() = false;
		r[3].value() = 100;

		auto highpoints = identifyHighPoints(r, 6);
		EXPECT_EQ(highpoints.size(), 2);

		auto& pointone = highpoints[0];
		auto& pointtwo = highpoints[1];

		std::set<cell_t> expectedCells = { 6,8 };
		EXPECT_TRUE(expectedCells.contains(pointone));
		EXPECT_TRUE(expectedCells.contains(pointtwo));
		EXPECT_NE(pointone, pointtwo);
	}
}