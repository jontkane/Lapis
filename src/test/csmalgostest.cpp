#include<gtest/gtest.h>
#include"..\csmalgos.hpp"

namespace lapis {
	TEST(CsmAlgosTest, smoothAndFillTest) {
		//The exact details of this function are likely to change over time. This test just ensures the following:
		//With smooth=3, if an output pixel has a value, that value is equal to the mean of the 3x3 area around it (if applicable)
		//If a pixel is in the center of a block of data, it ought to have a value

		bool T = true;
		bool F = false;
		std::vector<bool> hasValue = {
			F,F,F,F,F,F,F,
			F,T,T,T,T,T,F,
			F,T,F,T,T,T,F,
			F,T,T,T,T,T,F,
			F,T,T,F,F,T,F,
			F,T,T,T,T,T,F,
			F,F,F,F,F,F,F,
		};
		Raster<csm_t> r = Raster<csm_t>(Alignment(Extent(0, 7, 0, 7), 7, 7));
		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			auto v = r[cell];
			v.has_value() = hasValue[cell];
			v.value() = cell;
		}
		Raster<double> out = smoothAndFill(r, 3, 6, {});
		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			rowcol_t row = r.rowFromCell(cell);
			rowcol_t col = r.colFromCell(cell);
			if (row == 0 || row == r.nrow() - 1) {
				continue;
			}
			if (col == 0 || col == r.ncol() - 1) {
				continue;
			}
			EXPECT_TRUE(out[cell].has_value());
			csm_t numerator = 0; csm_t denominator = 0;
			for (rowcol_t br : {row - 1, row, row + 1}) {
				for (rowcol_t bc : {col - 1, col, col + 1}) {
					auto v = r.atRC(br, bc);
					if (v.has_value()) {
						denominator++;
						numerator += v.value();
					}
				}
			}
			EXPECT_NEAR(numerator / denominator, out.atRC(row, col).value(), 0.01);
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