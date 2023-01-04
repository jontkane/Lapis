#include"test_pch.hpp"
#include"..\gis\LasIO.hpp"


//if you used the conda install of lazperf, this test will only succeed in release mode

//insufficiently tested:
//synthetic
//keypoint
//withheld
//edge of flightline
//extra bytes
namespace lapis {
	TEST(LasIOTest, laz10) {
		std::string testfilefolder = std::string(LAPISTESTFILES);
		LasIO laz{ testfilefolder + "testlaz10.laz" };

		auto& h = laz.header;
		
		EXPECT_EQ(h.VersionMinor, 0);
		EXPECT_EQ(h.PointLength, 28);

		EXPECT_NEAR(h.ScaleFactor.x, 0.01,LAPIS_EPSILON);
		EXPECT_NEAR(h.ScaleFactor.y, 0.01, LAPIS_EPSILON);
		EXPECT_NEAR(h.ScaleFactor.z, 0.01, LAPIS_EPSILON);

		EXPECT_EQ(h.Offset.x, 2200000);
		EXPECT_EQ(h.Offset.y, 1100000);
		EXPECT_EQ(h.Offset.z, 0);

		EXPECT_NEAR(h.xmin, 2212594, 1);
		EXPECT_NEAR(h.xmax, 2212967, 1);
		EXPECT_NEAR(h.ymin, 1190539, 1);
		EXPECT_NEAR(h.ymax, 1190995, 1);
		EXPECT_NEAR(h.zmin, 1282, 1);
		EXPECT_NEAR(h.zmax, 1287, 1);

		EXPECT_EQ(h.NumberOfPoints(), 2139);
		EXPECT_FALSE(h.isWKT());
		EXPECT_EQ(h.PointFormat(), 1);
		EXPECT_TRUE(h.isCompressed());

		std::vector<coord_t> xExpected = { 2212751,2212753,2212752 };
		std::vector<coord_t> yExpected = { 1190985,1190947,1190931 };
		std::vector<coord_t> zExpected = { 1286,1286,1286 };
		std::vector<uint16_t> intExpected = { 56,53,33 };
		std::vector<uint8_t> retNumExpected = { 1,1,1 };
		std::vector<uint8_t> numRetsExpected = { 1,1,1 };
		std::vector<bool> scanDirectionFlagExpected = { 0,1,0 };
		std::vector<bool> edgeOfFlightlineExpected = { false,false,false };
		std::vector<double> scanAngleExpected = { 2,3,3 };
		std::vector<uint8_t> classificationExpected = { 2,2,2 };
		std::vector<bool> syntheticExpected = { false,false,false };
		std::vector<bool> keypointExpected = { false,false,false };
		std::vector<bool> withheldExpected = { false,false,false };

		std::array<char, 256> buffer;
		for (int i = 0; i < 3; ++i) {
			laz.readPoint(buffer.data());
			LasPoint10* point = (LasPoint10*)(buffer.data());

			EXPECT_NEAR(point->x * h.ScaleFactor.x + h.Offset.x, xExpected[i], 1);
			EXPECT_NEAR(point->y * h.ScaleFactor.y + h.Offset.y, yExpected[i], 1);
			EXPECT_NEAR(point->z * h.ScaleFactor.z + h.Offset.z, zExpected[i], 1);

			EXPECT_EQ(point->intensity, intExpected[i]);
			EXPECT_EQ(point->returnNumber(), retNumExpected[i]);
			EXPECT_EQ(point->numberOfReturns(), numRetsExpected[i]);
			EXPECT_EQ(point->scanDirectionFlag(), scanDirectionFlagExpected[i]);
			EXPECT_EQ(point->edgeOfFlightline(), edgeOfFlightlineExpected[i]);
			EXPECT_EQ(point->scanAngle(), scanAngleExpected[i]); //exact equality is fine for point10, point14 will want approx equality
			EXPECT_EQ(point->classification(), classificationExpected[i]);
			EXPECT_EQ(point->synthetic(), syntheticExpected[i]);
			EXPECT_EQ(point->keypoint(), keypointExpected[i]);
			EXPECT_EQ(point->withheld(), withheldExpected[i]);

		}
	}

	TEST(LasIOTest, laz14) {
		std::string testfilefolder = std::string(LAPISTESTFILES);
		LasIO laz{ testfilefolder + "testlaz14.laz" };

		auto& h = laz.header;

		EXPECT_EQ(h.VersionMinor, 4);
		EXPECT_EQ(h.PointLength, 30);

		EXPECT_NEAR(h.ScaleFactor.x, 0.01, LAPIS_EPSILON);
		EXPECT_NEAR(h.ScaleFactor.y, 0.01, LAPIS_EPSILON);
		EXPECT_NEAR(h.ScaleFactor.z, 0.01, LAPIS_EPSILON);

		EXPECT_EQ(h.Offset.x, 0);
		EXPECT_EQ(h.Offset.y, 0);
		EXPECT_EQ(h.Offset.z, 0);

		EXPECT_NEAR(h.xmin, 299000, 1);
		EXPECT_NEAR(h.xmax, 299001, 1);
		EXPECT_NEAR(h.ymin, 4202000, 1);
		EXPECT_NEAR(h.ymax, 4202006, 1);
		EXPECT_NEAR(h.zmin, 3312, 1);
		EXPECT_NEAR(h.zmax, 3314, 1);

		EXPECT_EQ(h.NumberOfPoints(), 93);
		EXPECT_TRUE(h.isWKT());
		EXPECT_EQ(h.PointFormat(), 6);
		EXPECT_TRUE(h.isCompressed());

		std::vector<coord_t> xExpected = { 299000,299000,299000 };
		std::vector<coord_t> yExpected = { 4202005,4202005,4202005 };
		std::vector<coord_t> zExpected = { 3312,3313,3312 };
		std::vector<uint16_t> intExpected = { 16485,10586,30045 };
		std::vector<uint8_t> retNumExpected = { 2,1,2 };
		std::vector<uint8_t> numRetsExpected = { 2,2,2 };
		std::vector<bool> scanDirectionFlagExpected = { 1,1,1 };
		std::vector<bool> edgeOfFlightlineExpected = { false,false,false };
		std::vector<double> scanAngleExpected = { 25,25,25 };
		std::vector<uint8_t> classificationExpected = { 2,3,2 };
		std::vector<bool> syntheticExpected = { false,false,false };
		std::vector<bool> keypointExpected = { false,false,false };
		std::vector<bool> withheldExpected = { false,false,false };

		std::array<char, 256> buffer;

		for (int i = 0; i < 12; ++i) { //there's more variety in points 12-14 than in 0-2
			laz.readPoint(buffer.data());
		}

		for (int i = 0; i < 3; ++i) {
			laz.readPoint(buffer.data());
			LasPoint14* point = (LasPoint14*)(buffer.data());

			EXPECT_NEAR(point->x * h.ScaleFactor.x + h.Offset.x, xExpected[i], 1);
			EXPECT_NEAR(point->y * h.ScaleFactor.y + h.Offset.y, yExpected[i], 1);
			EXPECT_NEAR(point->z * h.ScaleFactor.z + h.Offset.z, zExpected[i], 1);

			EXPECT_EQ(point->intensity, intExpected[i]);
			EXPECT_EQ(point->returnNumber(), retNumExpected[i]);
			EXPECT_EQ(point->numberOfReturns(), numRetsExpected[i]);
			EXPECT_EQ(point->scanDirectionFlag(), scanDirectionFlagExpected[i]);
			EXPECT_EQ(point->edgeOfFlightline(), edgeOfFlightlineExpected[i]);
			EXPECT_NEAR(point->scanAngle(), scanAngleExpected[i],1);
			EXPECT_EQ(point->classification(), classificationExpected[i]);
			EXPECT_EQ(point->synthetic(), syntheticExpected[i]);
			EXPECT_EQ(point->keypoint(), keypointExpected[i]);
			EXPECT_EQ(point->withheld(), withheldExpected[i]);

		}
	}
}