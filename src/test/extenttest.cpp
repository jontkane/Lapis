#include"..\gis\extent.hpp"
#include<gtest/gtest.h>

namespace lapis {
	void extentSame(const Extent& actual, const Extent& expected) {
		EXPECT_NEAR(actual.xmin(), expected.xmin(), 1.);
		EXPECT_NEAR(actual.xmax(), expected.xmax(), 1.);
		EXPECT_NEAR(actual.ymin(), expected.ymin(), 1.);
		EXPECT_NEAR(actual.ymax(), expected.ymax(), 1.);
	}
	
	TEST(ExtentTest, manualConstructor) {
		Extent manual{ -1,100,50,200,CoordRef("6340") };
		EXPECT_EQ(manual.xmin(), -1);
		EXPECT_EQ(manual.xmax(), 100);
		EXPECT_EQ(manual.ymin(), 50);
		EXPECT_EQ(manual.ymax(), 200);
		EXPECT_TRUE(manual.crs().isSame(CoordRef("6340")));
	}

	TEST(ExtentTest, constructFromFile) {
		std::string testfilefolder = std::string(LAPISTESTFILES);
		std::vector<std::string> sources{
			testfilefolder + "testlaz10.laz" // EPSG 2927+Unknown
			, testfilefolder + "testlaz14.laz" // EPSG 6340+5703
			, testfilefolder + "testraster.img" // EPSG 26911
			, testfilefolder + "testvect.shp" // EPSG 6340
		};
		std::vector<Extent> expected{
			Extent{2212593,2212967,1190539,1190995} //this crs is a pain in the ass to specify without using the full wkt
			, Extent{299000,299001,4202000,4202005,CoordRef("6340+5703")}
			, Extent{217971,218070,4185750,4185771,CoordRef{"26911"}}
			, Extent{217978,218069,4185751, 4185769,CoordRef("6340")}
		};
		for (int i = 0; i < sources.size(); ++i) {
			Extent e{ sources[i] };
			extentSame(e, expected[i]);
			EXPECT_FALSE(e.crs().isEmpty()) << "Failed on test " + std::to_string(i) << "Failed on test " + std::to_string(i);
			EXPECT_TRUE(e.crs().isConsistent(expected[i].crs())) << "Failed on test " + std::to_string(i) << "Failed on test " + std::to_string(i);
		}
	}

	TEST(ExtentTest, contains) {
		Extent e{ 0,100,200,300 };

		struct TestStruct {
			coord_t x, y;
		};
		std::vector<coord_t> xvals = { -50,0,50,100,250 }; //out,edge,in,edge,out
		std::vector<coord_t> yvals = { 50,200,250,300,350 };
		std::vector<TestStruct> points;
		std::vector<bool> normalExp;
		std::vector<bool> strictExp;

		for (int xidx = 0; xidx < xvals.size(); ++xidx) {
			for (int yidx = 0; yidx < yvals.size(); ++yidx) {
				points.push_back({ xvals[xidx],yvals[yidx] });
				normalExp.push_back(xidx > 0 && xidx < 4 && yidx>0 && yidx < 4);
				strictExp.push_back(xidx == 2 && yidx == 2 );
			}
		}

		for (int i = 0; i < points.size(); ++i) {
			coord_t x = points[i].x;
			coord_t y = points[i].y;
			EXPECT_TRUE(e.contains(x, y) == normalExp[i]) << "Failed on x=" + std::to_string(x) + ", y=" + std::to_string(y);
			EXPECT_TRUE(e.strictContains(x, y) == strictExp[i]) << "Failed on x=" + std::to_string(x) + ", y=" + std::to_string(y);
		}
	}

	TEST(ExtentTest, overlaps) {
		Extent e{ 0,100,200,300 };

		std::vector<Extent> tests = {
			{ 50,150,250,350 } //fully overlaps
			, { 100,200,250,350 } //touches X above
			, {-100,0,250,350} //touches X below
			, {50,150,300,400} //touches Y above
			, {50,150,100,200} //touches Y below
			, {100,200,300,400} //corner
			, {-100,200,200,400} //fully contains
			, {400,500,400,500} //no overlap
		};
		std::vector<bool> expNormal = {
			true
			, false
			, false
			, false
			, false
			, false
			, true
			, false
		};
		std::vector<bool> expTouch = {
			true
			, true
			, true
			, true
			, true
			, true
			, true
			, false
		};

		for (int i = 0; i < tests.size(); ++i) {
			EXPECT_EQ(e.overlaps(tests[i]), expNormal[i]) << "Failed on test " << std::to_string(i);
			EXPECT_EQ(e.touches(tests[i]), expTouch[i]) << "Failed on test  " << std::to_string(i);
		}
	}

	TEST(ExtentTest, extendCrop) {
		Extent base{ 0,100,0,100 };
		std::vector<Extent> tests = {
			Extent(25,75,25,75) //contained
			, Extent(-50,150,-50,150) //contains
			, Extent(50,150,50,150) //overlap
			, Extent(200,300,200,300) //no overlap
		};
		std::vector<Extent> expExtend = {
			Extent(0,100,0,100)
			, Extent(-50,150,-50,150)
			, Extent(0,150,0,150)
			, Extent(0,300,0,300)
		};
		std::vector<Extent> expCrop = {
			Extent(25,75,25,75)
			, Extent(0,100,0,100)
			, Extent(50,100,50,100)
			, Extent(0,0,0,0) //this one is supposed to throw so this is a placeholder
		};

		for (int i = 0; i < tests.size(); ++i) {
			extentSame(extendExtent(base, tests[i]), expExtend[i]);
			if (i < 3) {
				extentSame(cropExtent(base, tests[i]), expCrop[i]);
			}
			else {
				EXPECT_ANY_THROW(cropExtent(base, tests[i]));
			}
		}
	}
}