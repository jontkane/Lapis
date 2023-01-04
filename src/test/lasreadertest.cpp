#include"test_pch.hpp"
#include"..\gis\LasReader.hpp"

namespace lapis{
	TEST(LasReaderTest, getPoints) {
		std::string file = LAPISTESTFILES;
		file = file + "testlaz14.laz";
		LasReader lr{ file };

		EXPECT_EQ((Extent)lr, Extent(file));
		EXPECT_EQ((size_t)93, lr.nPoints());
		EXPECT_EQ((size_t)93, lr.nPointsRemaining());

		auto points = lr.getPoints(5);
		EXPECT_EQ(lr.nPointsRemaining(), 88);
		EXPECT_EQ((size_t)5, points.size());
		EXPECT_NEAR(3313.98, points[0].z, 0.01);
		EXPECT_NEAR(299000.3, points[2].x, 0.1);
		EXPECT_NEAR(4202003., points[4].y, 1.);
		EXPECT_EQ(47437, points[3].intensity);
		EXPECT_TRUE(points.crs.isSame(lr.crs()));

		auto nremain = lr.nPointsRemaining();
		points = lr.getPoints(100);
		EXPECT_EQ(nremain, points.size());
		EXPECT_EQ((size_t)0, lr.nPointsRemaining());
	}

	TEST(LasReaderTest, largeFile) {
		std::string file = std::string(LAPISTESTFILES) + "largelaz.laz";
		
		LasReader lr{ file };
		
		auto points = lr.getPoints(lr.nPoints());
		EXPECT_EQ(points.size(), 169873);
		EXPECT_NEAR(points[100000].z, 1317, 1);
	}

	TEST(LasReaderTest, filterFirstReturns) {
		std::string file = LAPISTESTFILES;
		file = file + "testlaz14.laz";
		LasReader lr{ file };

		lr.addFilter(std::make_shared<LasFilterFirstReturns>());
		auto points = lr.getPoints(100);
		EXPECT_EQ((size_t)91, points.size());
		EXPECT_NEAR(299000.3, points[50].x, 0.1);
	}

	TEST(LasReaderTest, classWhiteList) {
		std::string file = LAPISTESTFILES;
		file = file + "testlaz14.laz";
		LasReader lr{ file };

		std::unordered_set<std::uint8_t> set = { 1 };
		lr.addFilter(std::make_shared<LasFilterClassWhitelist>(set));
		auto points = lr.getPoints(100);
		EXPECT_EQ((size_t)60, points.size());
	}

	TEST(LasReaderTest, classBlackList) {
		std::string file = LAPISTESTFILES;
		file = file + "testlaz14.laz";
		LasReader lr = LasReader(file);

		std::unordered_set<std::uint8_t> set = { 1 };
		lr.addFilter(std::make_shared<LasFilterClassBlacklist>(set));
		auto points = lr.getPoints(100);
		EXPECT_EQ((size_t)33, points.size());
	}

	TEST(LasReaderTest, withheld) {
		std::string file = LAPISTESTFILES;
		file = file + "testwithwithheld.laz";
		LasReader lr = LasReader(file);
		lr.addFilter(std::make_shared<LasFilterWithheld>());
		auto points = lr.getPoints(lr.nPoints());
		EXPECT_EQ((size_t)37777, points.size());
	}
}