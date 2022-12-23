#include<gtest/gtest.h>
#include"..\demalgos.hpp"
#include"..\LapisData.hpp"

namespace lapis {

	LapisData& data() {
		return LapisData::getDataObject();
	}

	void prepareParams(const std::vector<std::string>& args) {
		data().resetObject();
		data().parseArgs(args);
		data().importBoostAndUpdateUnits();
		data().prepareForRun();
	}

	void prepareParamsNoSlowStuff(std::vector<std::string> args) {
		args.push_back("--debug-no-alignment");
		args.push_back("--debug-no-output");
		prepareParams(args);
	}

	TEST(DemAlgoTest, vendorrastertest) {
		std::vector<std::string> args;
		std::string testfilefolder = LAPISTESTFILES;
		args.push_back("--dem=" + testfilefolder + "finegroundmodeltest.img");
		args.push_back("--dem=" + testfilefolder + "coarsegroundmodeltest.img");
		args.push_back("--minht=0");
		args.push_back("--maxht=100");

		prepareParamsNoSlowStuff(args);

		//the fine raster is Alignment(0,5,0,5,5,5) and has a universal value of 1
		//the coarse raster is Alignment(0,10,0,10,2,2) and have a universal value of 10
		//as long as you don't extract too close to the boundary between them, you can expect a value of either 1 or 10 out

		LidarPointVector lpv{ "2927" };
		lpv.push_back({ 0.5,0.5,11,0,0 }); //normalizes to 10 in the fine raster and 1 in the coarse raster. expected: 10
		lpv.push_back({ 7.5,7.5,11,0,0 }); //can't be normalized by the fine raster, normalizes to 1 in the coarse raster. expected: 1
		lpv.push_back({ 0.5,0.5,102,0,0 }); //normalizes to 101 in the fine raster and 92 in the coarse raster. expected: filtered
		lpv.push_back({ 0.5,0.5,500,0,0 }); //far too high. expected:filtered
		lpv.push_back({ 12,12,20,0,0 }); //inside the extent, but not covered by either raster. expected: filtered
		
		Extent e{ 0,15,0,15,"2927"};

		VendorRaster vr;
		PointsAndDem pad = vr.normalizeToGround(lpv, e);

		ASSERT_EQ(pad.points.size(), 2);
		EXPECT_NEAR(pad.points[0].z, 10, 0.1);
		EXPECT_NEAR(pad.points[1].z, 1, 0.1);

		EXPECT_EQ(pad.dem.xres(), 1);
		EXPECT_EQ(pad.dem.yres(), 1);
		EXPECT_LE(pad.dem.xmin(), e.xmin());
		EXPECT_GE(pad.dem.xmax(), e.xmax());
		EXPECT_LE(pad.dem.ymin(), e.ymin());
		EXPECT_GE(pad.dem.ymax(), e.ymax());
		
		auto v = pad.dem.extract(2, 2,ExtractMethod::near);
		EXPECT_TRUE(v.has_value());
		EXPECT_EQ(v.value(), 1);
		v = pad.dem.extract(9, 9, ExtractMethod::near);
		EXPECT_TRUE(v.has_value());
		EXPECT_EQ(v.value(), 10);
		v = pad.dem.extract(10.5, 10.5, ExtractMethod::near);
		EXPECT_FALSE(v.has_value());

		//repojecting everything doesn't change the fundamental situation, and the results should be roughly the same.
		lpv.transform(CoordRef("2285"));
		e = QuadExtent(e, CoordRef("2285")).outerExtent();
		pad = vr.normalizeToGround(lpv, e);

		EXPECT_NEAR(pad.dem.xres(), 1, 0.1);
		EXPECT_NEAR(pad.dem.yres(), 1, 0.1);
		EXPECT_LE(pad.dem.xmin(), e.xmin());
		EXPECT_GE(pad.dem.xmax(), e.xmax());
		EXPECT_LE(pad.dem.ymin(), e.ymin());
		EXPECT_GE(pad.dem.ymax(), e.ymax());

		CoordTransform transform{ "2927","2285" };
		CoordXY xy = transform.transformSingleXY(2, 2);
		v = pad.dem.extract(xy.x, xy.y, ExtractMethod::near);
		EXPECT_TRUE(v.has_value());
		EXPECT_EQ(v.value(), 1);
		xy = transform.transformSingleXY(9, 9);
		v = pad.dem.extract(xy.x, xy.y, ExtractMethod::near);
		EXPECT_TRUE(v.has_value());
		EXPECT_EQ(v.value(), 10);
		xy = transform.transformSingleXY(10.5, 10.5);
		v = pad.dem.extract(xy.x, xy.y, ExtractMethod::near);
		EXPECT_FALSE(v.has_value());

		ASSERT_EQ(pad.points.size(), 2);
		EXPECT_NEAR(pad.points[0].z, 10, 0.1);
		EXPECT_NEAR(pad.points[1].z, 1, 0.1);
	}
}