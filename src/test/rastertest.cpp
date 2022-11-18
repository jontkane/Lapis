#include"..\gis\Raster.hpp"
#include<gtest/gtest.h>

namespace lapis {

	class RasterTest : public ::testing::Test {
	public:
		Raster<int> r;

		void SetUp() override {
			Alignment a(Extent(0, 5, 10, 15), 5, 5);
			Raster<int> r{ a };

			//the only way to actually set the values for testing is to use the functions that we're testing here, so this is also an implicit test of operator[]
			for (cell_t cell = 0; cell < a.ncell(); ++cell) {
				r[cell].value() = (int)cell;
				r[cell].has_value() = (cell % 2 == 0);
			}
		}
	};

	template<class T>
	void verifyRaster(Raster<T> r, std::vector<T> expVal, std::vector<bool> expHasVal) {
		EXPECT_EQ(r.ncell(), (cell_t)expVal.size());
		EXPECT_EQ(r.ncell(), (cell_t)expHasVal.size());

		for (size_t i = 0; i < expVal.size(); ++i) {
			if (expHasVal[i]) {
				EXPECT_TRUE(r[i].has_value());
				EXPECT_EQ(expVal[i], r[i].value());
			}
			else {
				EXPECT_FALSE(r[i].has_value());
			}
		}
	}

	TEST_F(RasterTest, constructorFromFile) {
		std::string file = LAPISTESTFILES;
		file += "/testraster.img";
		Raster<int> r{ file };
		EXPECT_FALSE(r[0].has_value());
		EXPECT_EQ(135, r[2457].value());
		EXPECT_TRUE(r[2457].has_value());

		Extent e{ (r.xmin() + r.xmax()) / 2,r.xmax(),(r.ymin() + r.ymax()) / 2,r.ymax() };
		r = Raster<int>(file, e);
		EXPECT_FALSE(r[0].has_value());
		EXPECT_EQ(145, r[915].value());
		EXPECT_TRUE(r[915].has_value());
	}

	TEST_F(RasterTest, atCell) {

		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			auto v = r.atCell(cell);
			EXPECT_EQ(v.value(), cell);
			EXPECT_EQ(v.has_value(), cell % 2 == 0);
			
			v.value() = 100;
			v.has_value() = !v.has_value();

			EXPECT_EQ(r.atCell(cell).value(), 100);
			EXPECT_EQ(r.atCell(cell).has_value(), cell % 2 != 0);

			v.value() = (int)cell;
			v.has_value() = !v.has_value();
		}

		EXPECT_ANY_THROW(r.atCell(-1));
		EXPECT_ANY_THROW(r.atCell(r.ncell()));
	}

	TEST_F(RasterTest, atCellUnsafe) {

		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			auto v = r.atCellUnsafe(cell);
			EXPECT_EQ(v.value(), cell);
			EXPECT_EQ(v.has_value(), cell % 2 == 0);

			v.value() = 100;
			v.has_value() = !v.has_value();

			EXPECT_EQ(r.atCellUnsafe(cell).value(), 100);
			EXPECT_EQ(r.atCellUnsafe(cell).has_value(), cell % 2 != 0);

			v.value() = (int)cell;
			v.has_value() = !v.has_value();
		}
	}

	TEST_F(RasterTest, atRC) {
		for (rowcol_t row = 0; row < r.nrow(); ++row) {
			for (rowcol_t col = 0; col < r.ncol(); ++col) {
				cell_t cell = r.cellFromRowCol(row, col);
				auto v = r.atRC(row,col);
				EXPECT_EQ(v.value(), cell);
				EXPECT_EQ(v.has_value(), cell % 2 == 0);

				v.value() = 100;
				v.has_value() = !v.has_value();

				EXPECT_EQ(r.atRC(row,col).value(), 100);
				EXPECT_EQ(r.atRC(row,col).has_value(), cell % 2 != 0);

				v.value() = (int)cell;
				v.has_value() = !v.has_value();
			}
		}

		EXPECT_ANY_THROW(r.atRC(-1, 0));
		EXPECT_ANY_THROW(r.atRC(r.nrow(), 0));
		EXPECT_ANY_THROW(r.atRC(0, -1));
		EXPECT_ANY_THROW(r.atRC(0, r.ncol()));
	}

	TEST_F(RasterTest, atRCUnsafe) {
		for (rowcol_t row = 0; row < r.nrow(); ++row) {
			for (rowcol_t col = 0; col < r.ncol(); ++col) {
				cell_t cell = r.cellFromRowCol(row, col);
				auto v = r.atRCUnsafe(row, col);
				EXPECT_EQ(v.value(), cell);
				EXPECT_EQ(v.has_value(), cell % 2 == 0);

				v.value() = 100;
				v.has_value() = !v.has_value();

				EXPECT_EQ(r.atRCUnsafe(row, col).value(), 100);
				EXPECT_EQ(r.atRCUnsafe(row, col).has_value(), cell % 2 != 0);

				v.value() = (int)cell;
				v.has_value() = !v.has_value();
			}
		}
	}

	TEST_F(RasterTest, atXY) {
		for (coord_t x = r.xmin()+r.xres()/2; x < r.xmax(); x += r.xres()) {
			for (coord_t y = r.ymin()+r.yres()/2; y < r.ymax(); y += r.yres()) {
				cell_t cell = r.cellFromXY(x, y);
				auto v = r.atXY(x, y);
				EXPECT_EQ(v.value(), cell);
				EXPECT_EQ(v.has_value(), cell % 2 == 0);

				v.value() = 100;
				v.has_value() = !v.has_value();

				EXPECT_EQ(r.atXY(x, y).value(), 100);
				EXPECT_EQ(r.atXY(x, y).has_value(), cell % 2 != 0);

				v.value() = (int)cell;
				v.has_value() = !v.has_value();
			}
		}

		EXPECT_ANY_THROW(r.atXY(-1, 12.5));
		EXPECT_ANY_THROW(r.atXY(6, 12.5));
		EXPECT_ANY_THROW(r.atXY(2.5, 9));
		EXPECT_ANY_THROW(r.atXY(2.5, 16));
	}

	TEST_F(RasterTest, atXYUnsafe) {
		for (coord_t x = r.xmin() + r.xres() / 2; x < r.xmax(); x += r.xres()) {
			for (coord_t y = r.ymin() + r.yres() / 2; y < r.ymax(); y += r.yres()) {
				cell_t cell = r.cellFromXY(x, y);
				auto v = r.atXYUnsafe(x, y);
				EXPECT_EQ(v.value(), cell);
				EXPECT_EQ(v.has_value(), cell % 2 == 0);

				v.value() = 100;
				v.has_value() = !v.has_value();

				EXPECT_EQ(r.atXYUnsafe(x, y).value(), 100);
				EXPECT_EQ(r.atXYUnsafe(x, y).has_value(), cell % 2 != 0);

				v.value() = (int)cell;
				v.has_value() = !v.has_value();
			}
		}
	}

	TEST_F(RasterTest, writeRaster) {
		std::string dir = LAPISTESTFILES;
		Raster<int> r{ dir + "/testraster.img" };
		r.writeRaster(dir + "/testraster.tif");
		Raster<int> r2{ dir + "/testraster.tif" };
		EXPECT_EQ(r, r2);
	}

	TEST_F(RasterTest, arithmetic) {
		Raster<float> lhs{ Alignment(Extent(0,2,0,2),2,2) };
		Raster<float> rhs{ (Alignment)lhs };
		for (int i = 0; i < 4; ++i) {
			lhs[i].value() = (float)(i + 1);
			rhs[i].value() = (float)i;
			if (i != 2) {
				lhs[i].has_value() = true;
			}
			if (i != 3) {
				rhs[i].has_value() = true;
			}
		}

		std::vector<float> expVal = { 1,3,5,7 };
		std::vector<bool> expHasVal = { true, true, false, false };
		verifyRaster(lhs + rhs, expVal, expHasVal);
		Raster<float> scratch = lhs;
		scratch += rhs;
		verifyRaster(scratch, expVal, expHasVal);

		expVal = { 0,2,6,12 };
		verifyRaster(lhs * rhs, expVal, expHasVal);
		scratch = lhs;
		scratch *= rhs;
		verifyRaster(scratch, expVal, expHasVal);

		expVal = { 1,1,1,1 };
		verifyRaster(lhs - rhs, expVal, expHasVal);
		scratch = lhs;
		scratch -= rhs;
		verifyRaster(scratch, expVal, expHasVal);

		expVal = { -1,2,1.5f, 4.f / 3.f }; //first value is garbage, since it's div by 0
		expHasVal = { false, true, false, false };
		verifyRaster(lhs / rhs, expVal, expHasVal);
		scratch = lhs;
		scratch /= rhs;
		verifyRaster(scratch, expVal, expHasVal);

		expHasVal = { true,true,false,true };
		expVal = { 2,3,4,5 };
		verifyRaster(lhs + 1, expVal, expHasVal);
		verifyRaster(1 + lhs, expVal, expHasVal);

		expVal = { 0,1,2,3 };
		verifyRaster(lhs - 1, expVal, expHasVal);
		expVal = { 0,-1,-2,-3 };
		verifyRaster(1 - lhs, expVal, expHasVal);

		expVal = { 2,4,6,8 };
		verifyRaster(lhs * 2, expVal, expHasVal);
		verifyRaster(2 * lhs, expVal, expHasVal);

		expVal = { -1,6,3,2 };
		expHasVal = { false,true,true,false };
		verifyRaster(6 / rhs, expVal, expHasVal);
		expVal = { 0,1,2,3 };
		expHasVal = { true,true,true,false };
		verifyRaster(rhs / 1, expVal, expHasVal);
		expHasVal = { false,false,false,false };
		verifyRaster(rhs / 0, expVal, expHasVal);
	}

	TEST_F(RasterTest, crop) {
		Alignment a{ Extent(0,3,0,3),3,3 };
		Raster<int> r{ a };
		for (int i = 0; i < r.ncell(); ++i) {
			r[i].value() = i;
			r[i].has_value() = (i % 3 != 0);
		}

		Extent contained{ 0.1,1.9,0.1,1.9 };
		Extent overlaps{ -1,1.9,-1,1.9 };
		Extent noOverlap{ -2,-1,-2,-1 };

		auto rtest = crop(r, contained);
		EXPECT_EQ((Alignment)rtest, crop((Alignment)r, contained));
		std::vector<int> expVal = { 3,4,6,7 };
		std::vector<bool> expHasVal = { false,true,false,true };
		verifyRaster(rtest, expVal, expHasVal);

		rtest = crop(r, overlaps);
		EXPECT_EQ((Alignment)rtest, crop((Alignment)r, contained));
		verifyRaster(rtest, expVal, expHasVal);

		EXPECT_ANY_THROW(crop(r, noOverlap));
	}

	TEST_F(RasterTest, extend) {
		Alignment a{ Extent(0,2,0,2),2,2 };
		Raster<int> r{ a };
		for (int i = 0; i < r.ncell(); ++i) {
			r[i].value() = i;
			r[i].has_value() = (i % 3 != 0);
		}

		Extent contained{ 0.1,1.9,0.1,1.9 };
		Extent overlaps{ -1,1.9,-1,1.9 };
		Extent noOverlap{ -1,-0.1,-1,-0.1 };

		auto rtest = extend(r, contained);
		EXPECT_EQ(r, rtest);

		rtest = extend(r, overlaps);
		std::vector<int> expVal = { -1,0,1,-1,2,3,-1,-1,-1 };
		std::vector<bool> expHasVal = { false,false,true,false,true,false,false,false,false };
		EXPECT_EQ((Alignment)rtest, extend((Alignment)r, overlaps));
		verifyRaster(rtest, expVal, expHasVal);

		rtest = extend(r, noOverlap);
		EXPECT_EQ((Alignment)rtest, extend((Alignment)r, overlaps));
		verifyRaster(rtest, expVal, expHasVal);
	}

	TEST_F(RasterTest, extract) {
		Raster<double> r{ Alignment(Extent(0,2,0,2),2,2) };
		for (int i = 0; i < r.ncell(); ++i) {
			r[i].value() = i;
			r[i].has_value() = true;
		}
		using em = ExtractMethod;
		auto test = r.extract(-1, -1, em::near); //outside extent;
		EXPECT_FALSE(test.has_value());

		test = r.extract(0.5, 0.5, em::near);
		EXPECT_TRUE(test.has_value());
		EXPECT_EQ(2., test.value());

		test = r.extract(1, 0.5, em::near); //x coordinate is on the boundary between two cells. I don't really care which value gets returned in this case
		EXPECT_TRUE(test.has_value());
		EXPECT_TRUE(test.value() == 2. || test.value() == 3.);

		test = r.extract(-1, -1, em::bilinear);
		EXPECT_FALSE(test.has_value());

		test = r.extract(0.5, 0.5, em::bilinear); //no actual interpolation to do
		EXPECT_TRUE(test.has_value());
		EXPECT_EQ(2., test.value());

		test = r.extract(1, 1, em::bilinear);
		double exp = ((0. + 1.) / 2. + (2. + 3.) / 2.) / 2.;
		EXPECT_TRUE(test.has_value());
		EXPECT_NEAR(exp, test.value(), LAPIS_EPSILON);
	}
}