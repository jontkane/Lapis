#include"test_pch.hpp"
#include"..\gis\Alignment.hpp"

namespace lapis {

	void verifyAlignment(const Alignment& a, const Extent& expExt, rowcol_t expRows, rowcol_t expCols) {
		EXPECT_NEAR(a.xmin(), expExt.xmin(), LAPIS_EPSILON);
		EXPECT_NEAR(a.xmax(), expExt.xmax(), LAPIS_EPSILON);
		EXPECT_NEAR(a.ymin(), expExt.ymin(), LAPIS_EPSILON);
		EXPECT_NEAR(a.ymax(), expExt.ymax(), LAPIS_EPSILON);

		EXPECT_TRUE(a.crs().isConsistent(expExt.crs()));
		EXPECT_EQ(a.crs().isEmpty(), expExt.crs().isEmpty());

		EXPECT_EQ(a.nrow(), expRows);
		EXPECT_EQ(a.ncol(), expCols);
		EXPECT_EQ(a.ncell(), expRows * expCols);

		EXPECT_NEAR(a.xres(), (a.xmax() - a.xmin()) / a.ncol(), LAPIS_EPSILON);
		EXPECT_NEAR(a.yres(), (a.ymax() - a.ymin()) / a.nrow(), LAPIS_EPSILON);

		EXPECT_TRUE(a.xOrigin() >= 0);
		EXPECT_TRUE(a.yOrigin() >= 0);
		EXPECT_TRUE(a.xOrigin() < a.xres());
		EXPECT_TRUE(a.yOrigin() < a.yres());
	}

	TEST(AlignmentTest, rawConstructors) {

		Extent ext(0, 100, 200, 300, CoordRef("2927"));

		Alignment a;

		//from xmin/ymin/nrow/ncol/xres/yres
		a = Alignment(0, 200, 10, 20, 5, 10, CoordRef("2927"));
		verifyAlignment(a, ext, 10, 20);
		EXPECT_ANY_THROW(Alignment(0, 100, 10, 20, 0, 10));
		EXPECT_ANY_THROW(Alignment(0, 100, 10, 20, -1, 10));
		EXPECT_ANY_THROW(Alignment(0, 100, 10, 20, 5, 0));
		EXPECT_ANY_THROW(Alignment(0, 100, 10, 20, 5, -1));
		EXPECT_ANY_THROW(Alignment(0, 100, -1, 20, 5, 10));
		EXPECT_ANY_THROW(Alignment(0, 100, 10, -1, 5, 10));

		//from extent/nrow/ncol
		a = Alignment(ext, 10, 20);
		verifyAlignment(a, ext, 10, 20);
		EXPECT_ANY_THROW(Alignment(ext, -1, 20));
		EXPECT_ANY_THROW(Alignment(ext, 10, -1));

		//from extent/xorigin/yorigin/xres/yres
		a = Alignment(ext, -1, -2, 5, 10);
		verifyAlignment(a, Extent(-1, 104, 198, 308, CoordRef("2927")), 11, 21);
		EXPECT_ANY_THROW(Alignment(ext, -1, -2, 0, 10));
		EXPECT_ANY_THROW(Alignment(ext, -1, -2, -1, 10));
		EXPECT_ANY_THROW(Alignment(ext, -1, -2, 5, 0));
		EXPECT_ANY_THROW(Alignment(ext, -1, -2, 5, -1));
	}

	TEST(AlignmentTest, fromFile) {
		std::string testfilefolder = std::string(LAPISTESTFILES);
		Alignment a{ testfilefolder + "testraster.img" };
		verifyAlignment(a, Extent(testfilefolder + "testraster.img"), 29, 132);
	}

	TEST(AlignmentTest, origin) {
		Extent ext(0, 100, 200, 300);
		Alignment a = Alignment(ext, -1, -2, 5, 10);
		EXPECT_NEAR(a.xOrigin(), 4, LAPIS_EPSILON);
		EXPECT_NEAR(a.yOrigin(), 8, LAPIS_EPSILON);
	}

	TEST(AlignmentTest, cellMath) {
		Alignment a(Extent(0, 100, 200, 300), 10, 20);

		//these are all describing the same location
		std::vector<cell_t> testCells = { 0,100,150 };
		std::vector<rowcol_t> testRows = { 0,5,7 };
		std::vector<rowcol_t> testCols = { 0,0,10 };
		std::vector<coord_t> testX = { 2.5,2.5,52.5 };
		std::vector<coord_t> testY = { 295,245,225 };

		for(int i = 0; i < testCells.size(); ++i) {
			cell_t cell = testCells[i];
			rowcol_t row = testRows[i];
			rowcol_t col = testCols[i];
			coord_t x = testX[i];
			coord_t y = testY[i];

			EXPECT_EQ(cell, a.cellFromRowCol(row, col));
			EXPECT_EQ(cell, a.cellFromXY(x, y));
			EXPECT_EQ(cell, a.cellFromRowColUnsafe(row, col));
			EXPECT_EQ(cell, a.cellFromXYUnsafe(x, y));

			EXPECT_EQ(row, a.rowFromCell(cell));
			EXPECT_EQ(row, a.rowFromY(y));
			EXPECT_EQ(row, a.rowFromCellUnsafe(cell));
			EXPECT_EQ(row, a.rowFromYUnsafe(y));

			EXPECT_EQ(col, a.colFromCell(cell));
			EXPECT_EQ(col, a.colFromX(x));
			EXPECT_EQ(col, a.colFromCellUnsafe(cell));
			EXPECT_EQ(col, a.colFromXUnsafe(x));

			EXPECT_NEAR(x, a.xFromCell(cell),LAPIS_EPSILON);
			EXPECT_NEAR(x, a.xFromCol(col), LAPIS_EPSILON);
			EXPECT_NEAR(x, a.xFromCellUnsafe(cell), LAPIS_EPSILON);
			EXPECT_NEAR(x, a.xFromColUnsafe(col), LAPIS_EPSILON);
			
			EXPECT_NEAR(y, a.yFromCell(cell), LAPIS_EPSILON);
			EXPECT_NEAR(y, a.yFromRow(row), LAPIS_EPSILON);
			EXPECT_NEAR(y, a.yFromCellUnsafe(cell), LAPIS_EPSILON);
			EXPECT_NEAR(y, a.yFromRowUnsafe(row), LAPIS_EPSILON);
		}

		EXPECT_ANY_THROW(a.cellFromRowCol(-1, 0));
		EXPECT_ANY_THROW(a.cellFromRowCol(a.nrow(), 0));
		EXPECT_ANY_THROW(a.cellFromRowCol(0, -1));
		EXPECT_ANY_THROW(a.cellFromRowCol(0, a.ncol()));

		EXPECT_ANY_THROW(a.cellFromXY(-1, 250));
		EXPECT_ANY_THROW(a.cellFromXY(101, 250));
		EXPECT_ANY_THROW(a.cellFromXY(50, 199));
		EXPECT_ANY_THROW(a.cellFromXY(50, 301));

		EXPECT_ANY_THROW(a.rowFromCell(-1));
		EXPECT_ANY_THROW(a.rowFromCell(a.ncell()));
		EXPECT_ANY_THROW(a.colFromCell(-1));
		EXPECT_ANY_THROW(a.colFromCell(a.ncell()));

		EXPECT_ANY_THROW(a.xFromCell(-1));
		EXPECT_ANY_THROW(a.xFromCell(a.ncell()));
		EXPECT_ANY_THROW(a.yFromCell(-1));
		EXPECT_ANY_THROW(a.yFromCell(a.ncell()));

		EXPECT_ANY_THROW(a.xFromCol(-1));
		EXPECT_ANY_THROW(a.xFromCol(a.ncol()));
		EXPECT_ANY_THROW(a.yFromRow(-1));
		EXPECT_ANY_THROW(a.yFromRow(a.nrow()));
	}

	TEST(AlignmentTest, alignExtent) {
		Alignment a{ Extent(0,100,0,100),10,10 };

		Extent e{ 2,103,-1,98 };
		Extent aligned = a.alignExtent(e, SnapType::near);
		EXPECT_EQ(Extent(0,100,0,100), aligned);

		aligned = a.alignExtent(e, SnapType::in);
		Extent exp{ 10,100,0,90 };
		EXPECT_EQ(exp, aligned);

		aligned = a.alignExtent(e, SnapType::out);
		exp = Extent(0, 110, -10, 100);
		EXPECT_EQ(exp, aligned);

		aligned = a.alignExtent(e, SnapType::ll);
		exp = Extent(0, 100, -10, 90);
		EXPECT_EQ(exp, aligned);
	}

	TEST(AlignmentTest, cellsFromExtent) {
		Alignment a{ Extent(0,100,0,100),10,10 };
		Extent e{ 2,18,2,19 };
		std::vector<cell_t> cells = a.cellsFromExtent(e, SnapType::near);

		std::set<cell_t> exp = { 80,81,90,91 };

		std::set<cell_t> found;
		for (cell_t cell : cells) {
			EXPECT_TRUE(exp.contains(cell));
			EXPECT_FALSE(found.contains(cell));
			found.insert(cell);
		}
		EXPECT_EQ(found.size(), 4);

		found.clear();
		for (cell_t cell : a.cellsFromExtentIterator(e, SnapType::near)) {
			EXPECT_TRUE(exp.contains(cell));
			EXPECT_FALSE(found.contains(cell));
			found.insert(cell);
		}
		EXPECT_EQ(found.size(), 4);

	}

	TEST(AlignmentTest, rowColExtent) {
		auto a = Alignment(Extent(0, 100, 0, 100, CoordRef("2967")), 10, 10);
		auto e = Extent(0, 30, 0, 30);
		auto rc = a.rowColExtent(e, SnapType::near);
		EXPECT_EQ(0, rc.mincol);
		EXPECT_EQ(2, rc.maxcol);
		EXPECT_EQ(7, rc.minrow);
		EXPECT_EQ(9, rc.maxrow);

		e = Extent(-50, 30, -50, 30);
		rc = a.rowColExtent(e, SnapType::near);
		EXPECT_EQ(0, rc.mincol);
		EXPECT_EQ(2, rc.maxcol);
		EXPECT_EQ(7, rc.minrow);
		EXPECT_EQ(9, rc.maxrow);

		e = Extent(200, 300, 200, 300);
		EXPECT_ANY_THROW(a.rowColExtent(e, SnapType::near));

		e = Extent(0, 30, 0, 30, CoordRef("2966"));
		EXPECT_ANY_THROW(a.rowColExtent(e, SnapType::near));

		rc = a.rowColExtent(a, SnapType::near);
		EXPECT_EQ(0, rc.mincol);
		EXPECT_EQ(a.ncol() - 1, rc.maxcol);
		EXPECT_EQ(0, rc.minrow);
		EXPECT_EQ(a.nrow() - 1, rc.maxrow);
	}

	TEST(AlignmentTest, extentFromCell) {
		Alignment a{ Extent(0, 100, 0, 100), 10, 10 };

		Extent test = a.extentFromCell(12);
		Extent exp{ 20,30,80,90 };
		EXPECT_EQ(test, exp);
	}

	TEST(AlignmentTest, crop) {
		Alignment a{ 0, 0, 10, 10, 10, 10 };
		Extent contained{ 20, 40, 30, 50 }; // contained in a
		Extent overlaps{ -10, 40, 30, 110 }; // overlaps a
		Extent nooverlap{ 150, 200, 150, 200 }; // does not overlap a
		Extent contains{ -10, 110, -10, 110 }; // contains a
		Extent misaligned{ 19.9, 40.1, 30.1, 59.9 }; // slightly misaligned
		Extent wrongproj{ 20, 40, 30, 50, CoordRef("2966") }; // wrong projection

		Alignment toTest = cropAlignment(a, contained, SnapType::near);
		verifyAlignment(toTest, Extent(20, 40, 30, 50), 2, 2);

		toTest = cropAlignment(a, overlaps, SnapType::near);
		verifyAlignment(toTest, Extent(0, 40, 30, 100), 7, 4);

		EXPECT_ANY_THROW(cropAlignment(a, nooverlap, SnapType::near));

		toTest = cropAlignment(a, contains, SnapType::near);
		verifyAlignment(toTest, Extent(0, 100, 0, 100), 10, 10);

		toTest = cropAlignment(a, misaligned, SnapType::near);
		verifyAlignment(toTest, Extent(20, 40, 30, 60), 3, 2);

		toTest = cropAlignment(a, misaligned, SnapType::out);
		verifyAlignment(toTest, Extent(10, 50, 30, 60), 3, 4);

		toTest = cropAlignment(a, misaligned, SnapType::in);
		verifyAlignment(toTest, Extent(20, 40, 40, 50), 1, 2);

		toTest = cropAlignment(a, misaligned, SnapType::ll);
		verifyAlignment(toTest, Extent(10, 40, 30, 50), 2, 3);

		EXPECT_ANY_THROW(cropAlignment(Alignment(0, 0, 10, 10, 10, 10, CoordRef("2967")), wrongproj, SnapType::near));
	}

	TEST(AlignmentTest, extendAlignment) {
		Alignment a{ 0, 0, 10, 10, 10, 10 };
		Extent overlaps{ 50, 150, 50, 150 }; // overlaps a
		Extent contained{ 10, 90, 10, 90 }; // contained in a
		Extent contains{ -10, 110, -10, 110 }; // contains a
		Extent nooverlap{ 110, 150, 110, 150 }; // does not overlap a
		Extent misalign{ 49.9, 150.1, 50.1, 149.9 }; // slightly misaligned
		Extent wrongproj{ 50, 150, 50, 150, CoordRef("2966") }; // wrong projection

		Alignment toTest = extendAlignment(a, overlaps, SnapType::near);
		verifyAlignment(toTest, Extent(0, 150, 0, 150), 15, 15);

		toTest = extendAlignment(a, contained, SnapType::near);
		verifyAlignment(toTest, Extent(0, 100, 0, 100), 10, 10);

		toTest = extendAlignment(a, contains, SnapType::near);
		verifyAlignment(toTest, Extent(-10, 110, -10, 110), 12, 12);

		toTest = extendAlignment(a, nooverlap, SnapType::near);
		verifyAlignment(toTest, Extent(0, 150, 0, 150), 15, 15);

		toTest = extendAlignment(a, misalign, SnapType::near);
		verifyAlignment(toTest, Extent(0, 150, 0, 150), 15, 15);

		toTest = extendAlignment(a, misalign, SnapType::in);
		verifyAlignment(toTest, Extent(0, 150, 0, 140), 14, 15);

		toTest = extendAlignment(a, misalign, SnapType::out);
		verifyAlignment(toTest, Extent(0, 160, 0, 150), 15, 16);

		toTest = extendAlignment(a, misalign, SnapType::ll);
		verifyAlignment(toTest, Extent(0, 150, 0, 140), 14, 15);

		EXPECT_ANY_THROW(extendExtent(Alignment(0, 0, 10, 10, 10, 10, CoordRef("2967")), wrongproj));
	}

	TEST(AlignmentTest, transformAlignment) {
		CoordRef feet = "2927";
		CoordRef meters = "32610";
		CoordRef degrees = "4326";

		Alignment a = Alignment{ 950000,1400000,10,10,10,10,feet }; //having an extent which is in the usable zone of the CRS is important for heuristic-based tests
		Alignment transformed = a.transformAlignment(feet);
		EXPECT_EQ(a, transformed);

		transformed = a.transformAlignment(CoordRef());
		EXPECT_EQ(a.xmin(), transformed.xmin());
		EXPECT_EQ(a.xmax(), transformed.xmax());
		EXPECT_EQ(a.ymin(), transformed.ymin());
		EXPECT_EQ(a.ymax(), transformed.ymax());
		EXPECT_EQ(a.ncol(), transformed.ncol());
		EXPECT_EQ(a.nrow(), transformed.nrow());
		EXPECT_TRUE(transformed.crs().isEmpty());

		transformed = a.transformAlignment(meters);
		EXPECT_NEAR(transformed.xres(), 10*0.3048, 0.5); //you can distort the cellsize a lot when switching projections, hence the large epsilon here
		EXPECT_EQ(transformed.xres(), transformed.yres());
		EXPECT_EQ(a.ncol(), transformed.ncol());
		EXPECT_EQ(a.nrow(), transformed.nrow());
		Extent extentOnly = QuadExtent(a, meters).outerExtent();
		EXPECT_NEAR(transformed.xmin(), extentOnly.xmin(), 1); //these two algorithms can be expected to drift a bit but shouldn't ever be too far off
		EXPECT_NEAR(transformed.ymin(), extentOnly.ymin(), 1);

		transformed = a.transformAlignment(degrees);
		EXPECT_EQ(a.ncol(), transformed.ncol());
		EXPECT_EQ(a.nrow(), transformed.nrow());
		extentOnly = QuadExtent(a, degrees).outerExtent();
		EXPECT_NEAR(transformed.xmin(), extentOnly.xmin(), 0.001);
		EXPECT_NEAR(transformed.xmax(), extentOnly.xmax(), 0.001);
		EXPECT_NEAR(transformed.ymin(), extentOnly.ymin(), 0.001);
		EXPECT_NEAR(transformed.ymax(), extentOnly.ymax(), 0.001);


	}
}