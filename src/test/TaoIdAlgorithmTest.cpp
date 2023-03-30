#include"test_pch.hpp"
#include"..\algorithms\AllTaoIdAlgorithms.hpp"

namespace lapis {

	TEST(TaoAlgorithmTest, highPointsTest) {
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

		HighPoints algo(6, 0);

		auto highpoints = algo.identifyTaos(r);
		EXPECT_EQ(highpoints.size(), 2);

		auto& pointone = highpoints[0];
		auto& pointtwo = highpoints[1];

		std::set<cell_t> expectedCells = { 6,8 };
		EXPECT_TRUE(expectedCells.contains(pointone));
		EXPECT_TRUE(expectedCells.contains(pointtwo));
		EXPECT_NE(pointone, pointtwo);
	}

	TEST(TaoAlgorithmTest, highPointsWithMinDistTest) {
		Raster<csm_t> r{ Alignment(Extent(0,3,0,3),3,3) };
		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			r[cell].has_value() = true;
			r[cell].value() = 5;
		}
		r[0].value() = 7;
		r[2].value() = 6;
		r[6].value() = 6;
		r[8].value() = 7;

		HighPoints algo(2, 2.1);

		auto highpoints = algo.identifyTaos(r);
		EXPECT_EQ(highpoints.size(), 2);

		auto& pointone = highpoints[0];
		auto& pointtwo = highpoints[1];

		std::set<cell_t> expectedCells = { 0,8 };
		EXPECT_TRUE(expectedCells.contains(pointone));
		EXPECT_TRUE(expectedCells.contains(pointtwo));
		EXPECT_NE(pointone, pointtwo);
	}

	TEST(TaoAlgorithmTest, highPointsTieBreakTest) {
		Raster<csm_t> r{ Alignment(Extent(0,3,0,3),3,3) };
		for (cell_t cell : CellIterator(r)) {
			r[cell].has_value() = true;
			r[cell].value() = 10;
		}

		HighPoints algo(2, 0);
		auto highPoints = algo.identifyTaos(r);

		EXPECT_EQ(highPoints.size(), 1);
	}
}