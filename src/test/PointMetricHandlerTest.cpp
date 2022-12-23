#include<gtest/gtest.h>
#include"..\ProductHandler.hpp"
#include"ParameterSpoofer.hpp"
#include"..\PointMetricCalculator.hpp"

namespace lapis {

	class PointMetricHandlerProtectedAccess : public PointMetricHandler {
	public:

		PointMetricHandlerProtectedAccess(ParamGetter* getter) : PointMetricHandler(getter) {}

		Raster<int>& nLaz() {
			return _nLaz;
		}
		unique_raster<PointMetricCalculator>& allReturnPMC() {
			return _allReturnPMC;
		}
		unique_raster<PointMetricCalculator>& firstReturnPMC() {
			return _firstReturnPMC;
		}
		std::vector<PointMetricRasters>& pointMetrics() {
			return _pointMetrics;
		}
		std::vector<StratumMetricRasters>& stratumMetrics() {
			return _stratumMetrics;
		}
	};

	void setReasonablePointMetricDefaults(PointMetricParameterSpoofer& spoof) {
		setReasonableSharedDefaults(spoof);
		spoof.setCanopyCutoff(2);
		spoof.setDoAdvancedPointMetrics(false);
		spoof.setDoAllReturnMetrics(true);
		spoof.setDoFirstReturnMetrics(true);
		spoof.setDoPointMetrics(true);
		spoof.setDoStratumMetrics(true);
		spoof.setMaxHt(100);
		spoof.setStrata({ 2,4,8 }, { "lt2","2-4","4-8","gt8" });
	}

	TEST(PointMetricHandlerTest, constructortest) {
		PointMetricParameterSpoofer spoof;
		setReasonablePointMetricDefaults(spoof);

		spoof.addLasExtent(Extent(0, 2, 0, 2));
		spoof.addLasExtent(Extent(1, 3, 1, 2));

		PointMetricHandlerProtectedAccess pmh(&spoof);

		std::vector<bool> expectedNLazHasValue = {
			false,false,false,
			true,true,true,
			true,true,false
		};
		std::vector<int> expectedNLazValue = {
			-9999,-9999,-9999,
			1,2,1,
			1,1,-9999
		};

		ASSERT_EQ((Alignment)pmh.nLaz(), *spoof.metricAlign());
		for (cell_t cell = 0; cell < pmh.nLaz().ncell(); ++cell) {
			EXPECT_EQ(expectedNLazHasValue[cell], pmh.nLaz()[cell].has_value());
			if (expectedNLazHasValue[cell]) {
				EXPECT_EQ(expectedNLazValue[cell], pmh.nLaz()[cell].value());
			}
		}

		ASSERT_TRUE(pmh.allReturnPMC());
		EXPECT_EQ((Alignment)*pmh.allReturnPMC(), *spoof.metricAlign());
		ASSERT_TRUE(pmh.firstReturnPMC());
		EXPECT_EQ((Alignment)*pmh.firstReturnPMC(), *spoof.metricAlign());

		EXPECT_GT(pmh.pointMetrics().size(), 0);
		size_t withoutAdvancedMetrics = pmh.pointMetrics().size();
		for (size_t i = 0; i < pmh.pointMetrics().size(); ++i) {
			EXPECT_TRUE(pmh.pointMetrics()[i].rasters.all.has_value());
			EXPECT_TRUE(pmh.pointMetrics()[i].rasters.first.has_value());
			EXPECT_EQ((Alignment)pmh.pointMetrics()[i].rasters.all.value(), *spoof.metricAlign());
			EXPECT_EQ((Alignment)pmh.pointMetrics()[i].rasters.first.value(), *spoof.metricAlign());
		}
		ASSERT_GT(pmh.stratumMetrics().size(), 0);
		EXPECT_EQ(pmh.stratumMetrics()[0].rasters.size(), spoof.strataBreaks().size() + 1);
		for (size_t i = 0; i < pmh.stratumMetrics().size(); ++i) {
			for (size_t j = 0; j < pmh.stratumMetrics()[i].rasters.size(); ++j) {
				EXPECT_TRUE(pmh.stratumMetrics()[i].rasters[j].all.has_value());
				EXPECT_EQ((Alignment)pmh.stratumMetrics()[i].rasters[j].all.value(), *spoof.metricAlign());
				EXPECT_TRUE(pmh.stratumMetrics()[i].rasters[j].first.has_value());
				EXPECT_EQ((Alignment)pmh.stratumMetrics()[i].rasters[j].first.value(), *spoof.metricAlign());
			}
		}

		spoof.setDoAdvancedPointMetrics(true);
		spoof.setDoAllReturnMetrics(false);
		spoof.setDoStratumMetrics(false);
		pmh = PointMetricHandlerProtectedAccess(&spoof);
		EXPECT_GT(pmh.pointMetrics().size(), withoutAdvancedMetrics);
		for (size_t i = 0; i < pmh.pointMetrics().size(); ++i) {
			EXPECT_FALSE(pmh.pointMetrics()[i].rasters.all.has_value());
			EXPECT_TRUE(pmh.pointMetrics()[i].rasters.first.has_value());
		}
		EXPECT_EQ(pmh.stratumMetrics().size(), 0);

		spoof.setDoAdvancedPointMetrics(false);
		spoof.setDoAllReturnMetrics(true);
		spoof.setDoFirstReturnMetrics(false);
		spoof.setDoStratumMetrics(true);
		pmh = PointMetricHandlerProtectedAccess(&spoof);
		for (size_t i = 0; i < pmh.pointMetrics().size(); ++i) {
			EXPECT_TRUE(pmh.pointMetrics()[i].rasters.all.has_value());
			EXPECT_FALSE(pmh.pointMetrics()[i].rasters.first.has_value());
		}
		for (size_t i = 0; i < pmh.stratumMetrics().size(); ++i) {
			for (size_t j = 0; j < pmh.stratumMetrics()[i].rasters.size(); ++j) {
				EXPECT_TRUE(pmh.stratumMetrics()[i].rasters[j].all.has_value());
				EXPECT_FALSE(pmh.stratumMetrics()[i].rasters[j].first.has_value());
			}
		}
	}

	TEST(PointMetricHandlerTest, handlepointstest) {
		PointMetricParameterSpoofer spoof;
		setReasonablePointMetricDefaults(spoof);

		Extent extentone = Extent(0, 2, 0, 2);
		Extent extenttwo = Extent(1, 3, 1, 2);

		spoof.addLasExtent(extentone);
		spoof.addLasExtent(extenttwo);

		PointMetricHandlerProtectedAccess pmh(&spoof);

		//pointmetriccalculator has its own test so I don't want the full suite here
		//I'm just verifying that the points make their way to the right PMC and that the functions get called at the appropriate time
		pmh.pointMetrics().clear();
		pmh.pointMetrics().emplace_back(&spoof, "CoverTest", &PointMetricCalculator::canopyCover, OutputUnitLabel::Percent);
		pmh.stratumMetrics().clear();
		pmh.stratumMetrics().emplace_back(&spoof, "PercentTest", &PointMetricCalculator::stratumPercent, OutputUnitLabel::Percent);

		LidarPointVector points;
		points.push_back(LasPoint{ 0.5,0.5,3,0,0 });
		points.push_back(LasPoint{ 0.5,1.5,1,0,0 });
		points.push_back(LasPoint{ 0.5,1.5,3,0,0 });
		points.push_back(LasPoint{ 1.5,1.5,3,0,0 });
		pmh.handlePoints(points, extentone, 0);

		std::vector<bool> expectedNLazHasValue = {
			false,false,false,
			true,true,true,
			true,true,false
		};
		std::vector<int> expectedNLazValue = {
			-9999,-9999,-9999,
			0,1,1,
			0,0,-9999
		};
		for (cell_t cell = 0; cell < pmh.nLaz().ncell(); ++cell) {
			EXPECT_EQ(expectedNLazHasValue[cell], pmh.nLaz()[cell].has_value());
			if (expectedNLazHasValue[cell]) {
				EXPECT_EQ(expectedNLazValue[cell], pmh.nLaz()[cell].value());
			}
		}

		std::vector<bool> expectedCoverHasValue = {
			false,false,false,
			true,false,false,
			true,false,false
		};
		std::vector<metric_t> expectedCoverValue = {
			-9999,-9999,-9999,
			50,-9999,-9999,
			100,-9999,-9999
		};
		Raster<metric_t>& cover = pmh.pointMetrics()[0].rasters.all.value();
		for (cell_t cell = 0; cell < cover.ncell(); ++cell) {
			EXPECT_EQ(expectedCoverHasValue[cell], cover[cell].has_value());
			if (expectedCoverHasValue[cell]) {
				EXPECT_EQ(expectedCoverValue[cell], cover[cell].value());
			}
		}

		Raster<metric_t>& stratum = pmh.stratumMetrics()[0].rasters[0].all.value();
		for (cell_t cell = 0; cell < stratum.ncell(); ++cell) {
			EXPECT_EQ(expectedCoverHasValue[cell], stratum[cell].has_value());
		}
	}

}