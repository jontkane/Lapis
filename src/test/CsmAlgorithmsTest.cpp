#include"test_pch.hpp"
#include"..\algorithms\AllCsmAlgorithms.hpp"

namespace lapis {

	TEST(CsmAlgorithmsTest, maxPointTest) {

		MaxPoint algo{ 2.99 }; //this diameter is large enough that each points will hit a 3x3 if dropepd in the center of a cell

		Raster<csm_t> expected{ Alignment(Extent(0,5,0,5),5,5) };
		LidarPointVector points;

		auto addPoint = [&](cell_t cell, coord_t value) {
			coord_t x = expected.xFromCell(cell);
			coord_t y = expected.yFromCell(cell);

			points.push_back(LasPoint{ x,y,value,0,0 });

			rowcol_t row = expected.rowFromCell(cell);
			rowcol_t col = expected.colFromCell(cell);

			for (rowcol_t nudgeRow : {row - 1, row, row + 1}) {
				if (nudgeRow < 0 || nudgeRow >= expected.nrow()) {
					continue;
				}
				for (rowcol_t nudgeCol : {col - 1, col, col + 1}) {
					if (nudgeCol < 0 || nudgeCol >= expected.ncol()) {
						continue;
					}
					auto v = expected.atRC(nudgeRow, nudgeCol);
					if (v.has_value()) {
						v.value() = std::max(v.value(), value);
					}
					else {
						v.value() = value;
						v.has_value() = true;
					}

				}
			}
		};

		addPoint(0, 5);
		addPoint(7, 3);
		addPoint(9, -1);
		addPoint(1, 4);

		auto csmMaker = algo.getCsmMaker(expected);
		csmMaker->addPoints(points);
		Raster<csm_t> actual = *csmMaker->currentCsm();

		ASSERT_TRUE(actual.xmin() <= expected.xmin());
		ASSERT_TRUE(actual.xmax() >= expected.xmax());
		ASSERT_TRUE(actual.ymin() <= expected.ymin());
		ASSERT_TRUE(actual.ymax() >= expected.ymax());

		for (cell_t cell = 0; cell < expected.ncell(); ++cell) {
			coord_t x = expected.xFromCell(cell);
			coord_t y = expected.yFromCell(cell);

			auto expV = expected[cell];
			auto actV = actual.atXY(x, y);

			EXPECT_EQ(expV.has_value(), actV.has_value());
			if (expV.has_value()) {
				EXPECT_NEAR(expV.value(), actV.value(), 0.01);
			}
		}
	}
}