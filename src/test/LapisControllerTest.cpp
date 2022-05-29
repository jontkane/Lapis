#include"..\LapisController.hpp"
#include<gtest/gtest.h>

namespace lapis {

	class LapisControllerDerived : public LapisController {
	public:
		LasProcessingObjects* lptr() {
			return lp;
		}
		GlobalProcessingObjects* gptr() {
			return gp;
		}

		LidarPointVector getPoints_test(size_t n) {
			return getPoints(n);
		}
	};

	TEST(LapisControllerTest, getPointsTest) {
		auto lc = LapisControllerDerived();
		std::string lazfile = std::string(LAPISTESTFILES) + "testlaz14.laz";
		std::string demfile = std::string(LAPISTESTFILES) + "testlazground.img";
		lc.gptr()->sortedLasFiles = { {lazfile,Extent(lazfile)} };
		lc.gptr()->demFiles = { {demfile,Alignment(demfile)} };
		lc.gptr()->minht = -100;
		lc.gptr()->maxht = 500;
		lc.lptr()->filters = { std::make_shared<LasFilterFirstReturns>() };

		auto p = lc.getPoints_test(0);
		EXPECT_EQ(p.size(), 91); //checking the filter was applied
		EXPECT_NEAR(0.09, p[1].z, 0.01); //checking the data was normalized

		lc.lptr()->calculators.setZUnits(linearUnitDefs::foot);
		p = lc.getPoints_test(0);
		EXPECT_EQ(p.size(), 91);
		EXPECT_NEAR(0.09/0.3048, p[1].z, 0.01);

		lc.lptr()->calculators.defineCRS(CoordRef("2927"));
		p = lc.getPoints_test(0);
		EXPECT_EQ(p.size(), 91);
		EXPECT_TRUE(p.crs.isConsistent(CoordRef("2927")));
		EXPECT_NEAR(p[0].x, 1993859, 1);
		EXPECT_NEAR(p[0].y, -2701350, 1);
	}
}