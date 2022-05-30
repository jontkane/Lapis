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

		void processCell_test(cell_t cell) {
			processCell(cell);
		}

		void assignPointsToCalculators_test(const LidarPointVector& points) {
			assignPointsToCalculators(points);
		}

		void assignPointsToCSM_test(const LidarPointVector& points, std::optional<Raster<csm_data>>& csm) {
			assignPointsToCSM(points, csm);
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

	TEST(LapisControllerTest, processCellTest) {
		LapisControllerDerived lc;
		Alignment a{ Extent(0,2,0,2),2,2 };
		lc.lptr()->calculators = Raster<PointMetricCalculator>(a);
		lc.lptr()->nLaz = Raster<int>(a);
		lc.lptr()->metricRasters.push_back(PointMetricRaster({ "returnCount",&PointMetricCalculator::returnCount }, a));
		lc.lptr()->cellMuts = std::vector<std::mutex>(lc.lptr()->mutexN);
		
		PointMetricCalculator::setInfo(2, 100, 0.1);
		for (int i = 0; i < lc.lptr()->nLaz.ncell(); ++i) {
			lc.lptr()->nLaz[i].value() = i+1;
			lc.lptr()->calculators[i].value().addPoint({ 0,0,0,0 });
		}
		for (int i = 0; i < lc.lptr()->nLaz.ncell(); ++i) {
			for (int j = 0; j < i; ++j) {
				EXPECT_FALSE(lc.lptr()->metricRasters[0].rast[i].has_value());
				lc.processCell_test(i);
			}
		}
		for (int i = 0; i < lc.lptr()->nLaz.ncell(); ++i) {
			lc.processCell_test(i);
			EXPECT_TRUE(lc.lptr()->metricRasters[0].rast[i].has_value());
		}
	}

	TEST(LapisControllerTest, assignPointsToCalculatorsTest) {
		LidarPointVector points;
		points.push_back({ 0.5,0.5,1,0 });
		points.push_back({ 0.5,0.5,1,0 });
		points.push_back({ 1.5,1.5,1,0 });

		PointMetricCalculator::setInfo(2, 100, 0.1);

		LapisControllerDerived lc;
		lc.lptr()->calculators = Raster<PointMetricCalculator>(Alignment(Extent(0, 2, 0, 2), 2, 2));
		lc.lptr()->cellMuts = std::vector<std::mutex>(lc.lptr()->mutexN);
		Raster<metric_t> valueHolder{ Alignment(Extent(0, 2, 0, 2), 2, 2) };
		

		lc.assignPointsToCalculators_test(points);

		lc.lptr()->calculators[0].value().returnCount(valueHolder, 0);
		EXPECT_EQ(valueHolder[0].value(), 0);

		lc.lptr()->calculators[1].value().returnCount(valueHolder, 1);
		EXPECT_EQ(valueHolder[1].value(), 1);

		lc.lptr()->calculators[2].value().returnCount(valueHolder, 2);
		EXPECT_EQ(valueHolder[2].value(), 2);

		lc.lptr()->calculators[3].value().returnCount(valueHolder, 3);
		EXPECT_EQ(valueHolder[3].value(), 0);
	}

	TEST(LapisControllerTest, assignPointsToCSMTest) {
		LidarPointVector points;
		points.push_back({ 0.5,0.5,1,0 });
		points.push_back({ 0.5,0.5,6,0 });
		points.push_back({ 1.5,1.5,2,0 });

		LapisControllerDerived lc;
		lc.lptr()->cellMuts = std::vector<std::mutex>(lc.lptr()->mutexN);

		std::optional<Raster<csm_data>> csm = Raster<csm_data>{ Alignment(Extent(0,2,0,2),2,2) };
		
		lc.assignPointsToCSM_test(points, csm);
		auto& csmv = csm.value();

		EXPECT_FALSE(csmv[0].has_value());
		
		EXPECT_TRUE(csmv[1].has_value());
		EXPECT_EQ(csmv[1].value(), 2);

		EXPECT_TRUE(csmv[2].has_value());
		EXPECT_EQ(csmv[2].value(), 6);

		EXPECT_FALSE(csmv[3].has_value());
	}
}