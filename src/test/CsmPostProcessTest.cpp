#include"test_pch.hpp"
#include"..\algorithms\AllCsmPostProcessors.hpp"

namespace lapis {

	TEST(CsmPostAlgosTest, doNothingTest) {
		Raster<csm_t> r = Raster<csm_t>(Alignment(Extent(0, 5, 0, 5), 5, 5));

		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			auto v = r[cell];
			v.value() = (csm_t)cell;
			v.has_value() = cell % 3 > 0;
		}

		DoNothingCsm algo;
		Raster<csm_t> out = algo.postProcess(r);
		
		ASSERT_TRUE(out.isSameAlignment(r));

		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			EXPECT_EQ(r[cell].has_value(), out[cell].has_value());
			if (r[cell].has_value()) {
				EXPECT_EQ(r[cell].value(), out[cell].value());
			}
		}
	}

	TEST(CsmPostAlgosTest, smoothFillTest) {

		//not going too hard on testing precise expected values because the details of this algorithm are likely to change in the future
		bool T = true;
		bool F = false;
		std::vector<bool> hasValue = {
			T,T,T,T,T,
			T,F,T,T,T,
			T,T,T,F,T,
			T,T,F,F,F,
			T,T,T,T,T,
		};
		Raster<csm_t> r = Raster<csm_t>(Alignment(Extent(0, 5, 0, 5), 5, 5));
		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			auto v = r[cell];
			v.has_value() = hasValue[cell];
			v.value() = (int)cell;
		}

		FillCsm fillAlgo{ 6,1.5 }; //all but one of the holes should get filled with these parameters
		SmoothCsm smoothAlgo{ 3 };
		SmoothAndFill fillSmoothAlgo{ 3,6,1.5 };

		Raster<csm_t> fillCsm = fillAlgo.postProcess(r);
		Raster<csm_t> smoothCsm = smoothAlgo.postProcess(r);
		Raster<csm_t> fillSmooth = fillSmoothAlgo.postProcess(r);

		ASSERT_TRUE(fillCsm.isSameAlignment(r));
		ASSERT_TRUE(smoothCsm.isSameAlignment(r));
		ASSERT_TRUE(fillSmooth.isSameAlignment(r));

		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			rowcol_t row = r.rowFromCell(cell);
			rowcol_t col = r.colFromCell(cell);

			csm_t numerator = 0; csm_t denominator = 0;
			for (rowcol_t br : {row - 1, row, row + 1}) {
				for (rowcol_t bc : {col - 1, col, col + 1}) {
					if (br < 0 || br >= r.nrow() || bc < 0 || bc >= r.ncol()) {
						continue;
					}
					auto v = r.atRC(br, bc);
					if (v.has_value()) {
						denominator++;
						numerator += v.value();
					}
				}
			}

			csm_t mean = numerator / denominator;

			if (r[cell].has_value()) {
				EXPECT_TRUE(fillCsm[cell].has_value());
				EXPECT_TRUE(smoothCsm[cell].has_value());
				EXPECT_TRUE(fillSmooth[cell].has_value());

				EXPECT_NEAR(smoothCsm[cell].value(), mean, 0.001);
				EXPECT_EQ(fillCsm[cell].value(), r[cell].value());
				EXPECT_EQ(fillSmooth[cell].value(), smoothCsm[cell].value());
			}
			else {
				EXPECT_FALSE(smoothCsm[cell].has_value());

				if (cell == 18 || cell == 13) {
					EXPECT_FALSE(fillCsm[cell].has_value());
					EXPECT_FALSE(fillSmooth[cell].has_value());
				}
				else {
					EXPECT_TRUE(fillCsm[cell].has_value());
					EXPECT_TRUE(fillSmooth[cell].has_value());

					EXPECT_NEAR(fillCsm[cell].value(), mean, 0.5); //a big epsilon because the real algorithm is doing a distance weight mean, not a simple mean
					EXPECT_EQ(fillCsm[cell].value(), fillSmooth[cell].value());
				}
			}
		}
	}
}