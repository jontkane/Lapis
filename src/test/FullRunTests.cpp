#include"test_pch.hpp"
#pragma warning(push)
#pragma warning(disable: 26495)
#include"..\Lapis.hpp"
#pragma warning(pop)

namespace lapis {

	TEST(FullRunTest, PointMetricsAndFilters) {

#pragma warning(push)
#pragma warning(disable: 4305)
#pragma warning(disable: 4267)

		std::string ini = LAPISTESTDATA + std::string("PointMetricsTest/params.ini");
		ASSERT_TRUE(std::filesystem::exists(ini)) << "Please run GenerateTestData.R first";
		std::vector<std::string> params = { "--ini-file=" + ini };
		ASSERT_EQ(lapisUnifiedMain(params), 0) << "Run Failed";

		//The data in this test should result in every cell having unfilitered points whose Z values are the integers from 0 to 50
		//There may be some slight deviation due to differences in how bilinear extraction is done in lapis vs the terra package in R
		//because the canopy cutoff is set to 1.5, canopy-only metrics should have the values from 2 to 50 instead

		namespace fs = std::filesystem;

		std::unordered_map<std::string, bool> filesChecked;
		for (auto& p : fs::recursive_directory_iterator(fs::path{ LAPISTESTDATA } / "PointMetricsTestOutput" / "PointMetrics")) {
			if (fs::is_regular_file(p.path())) {
				filesChecked[p.path().string()] = false;
			}
		}

		auto testRaster = [&](const fs::path& file, metric_t expectedValue) {
			ASSERT_TRUE(fs::exists(file));
			Raster<metric_t> r{ file.string() };
			for (cell_t cell : r.allCellsIterator()) {
				EXPECT_TRUE(r[cell].has_value());
				EXPECT_NEAR(r[cell].value(), expectedValue, 0.1) << file.string();
			}
			filesChecked[file.string()] = true;
		};

		//Some metrics are, unfortunately, extremely sensitive to float imprecisions with this sample dataset, and have multiple values depending on how the imprecision goes
		auto testMultipleExpected = [&](const fs::path& file, std::vector<metric_t> expected) {
			ASSERT_TRUE(fs::exists(file)) << file.string();
			Raster<metric_t> r{ file.string() };
			for (cell_t cell : r.allCellsIterator()) {
				EXPECT_TRUE(r[cell].has_value());
				metric_t minDiff = std::numeric_limits<metric_t>::max();
				for (metric_t v : expected) {
					minDiff = std::min(minDiff, std::abs(r[cell].value() - v));
				}
				EXPECT_LT(minDiff, 0.1) << file.string();
			}
			filesChecked[file.string()] = true;
		};

		fs::path allReturns = fs::path{ LAPISTESTDATA } / "PointMetricsTestOutput" / "PointMetrics" / "AllReturns";

		auto getPercentileName = [](const fs::path& parent, const std::string& p) {
			return parent / ("PointMetricsTest_" + p + "thPercentile_CanopyHeight_Meters.tif");
		};

		std::vector<std::string> pnames = { "05","10","15","20","25","30","35","40","45","50","55","60","65","70","75","80","85","90","95","99" };
		std::vector<metric_t> expectedPercentileAllReturns = { 4.4,6.8,9.2,11.6,14.0,16.4,18.8,21.2,23.6,26.0,28.4,30.8, 33.2,35.6,38.0,40.4,42.8,45.2,47.6,49.52 };

		for (size_t i = 0; i < pnames.size(); ++i) {
			testRaster(getPercentileName(allReturns, pnames[i]), expectedPercentileAllReturns[i]);
		}

		testRaster(allReturns / "PointMetricsTest_CanopyCover_Percent.tif", 96.078);
		testRaster(allReturns / "PointMetricsTest_CanopyKurtosis.tif", 1.799);
		testRaster(allReturns / "PointMetricsTest_CanopyReliefRatio.tif", 0.5);
		testRaster(allReturns / "PointMetricsTest_CanopySkewness.tif", 0);
		testMultipleExpected(allReturns / "PointMetricsTest_CoverAboveMean_Percent.tif", { 47.05,49.02 });
		testRaster(allReturns / "PointMetricsTest_Mean_CanopyHeight_Meters.tif", 26.);
		testRaster(allReturns / "PointMetricsTest_StdDev_CanopyHeight_Meters.tif", 14.14);
		testRaster(allReturns / "PointMetricsTest_TotalReturnCount.tif", 51);

		fs::path arStrata = allReturns / "StratumMetrics";
		std::vector<std::string> strataNames = { "LessThan0.5","0.5To5.5","5.5To10.5","GreaterThan10.5"};
		auto getStratumName = [&](const fs::path& parent, const std::string& metricName, int stratumNum) {
			return parent / ("PointMetricsTest_" + metricName + "_" + strataNames[stratumNum] + "Meters_Percent.tif");
		};


		std::vector<metric_t> expectedARCover = { 100,83.33,45.45,78.43 };
		std::vector<metric_t> expectedARPercent = { 1.96,9.8,9.8,78.4 };
		for (size_t i = 0; i < strataNames.size(); ++i) {
			testRaster(getStratumName(arStrata, "StratumCover", i), expectedARCover[i]);
			testRaster(getStratumName(arStrata, "StratumPercent", i), expectedARPercent[i]);
		}


		//for first return metrics, you can expect the values to be 0:20 (or 2:20 for canopy-only)

		fs::path firstReturns = fs::path{ LAPISTESTDATA } / "PointMetricsTestOutput" / "PointMetrics" / "FirstReturns";

		std::vector<metric_t> expectedPercentileFirstReturns = { 2.9,3.8,4.7,5.6,6.5,7.4,8.3,9.2,10.1,11.0,11.9,12.8,13.7,14.6,15.5,16.4,17.3,18.2,19.1,19.82 };

		for (size_t i = 0; i < pnames.size(); ++i) {
			testRaster(getPercentileName(firstReturns, pnames[i]), expectedPercentileFirstReturns[i]);
		}

		testRaster(firstReturns / "PointMetricsTest_CanopyCover_Percent.tif", 90.4);
		testRaster(firstReturns / "PointMetricsTest_CanopyKurtosis.tif", 1.793);
		testRaster(firstReturns / "PointMetricsTest_CanopyReliefRatio.tif", 0.5);
		testRaster(firstReturns / "PointMetricsTest_CanopySkewness.tif", 0);
		testMultipleExpected(firstReturns / "PointMetricsTest_CoverAboveMean_Percent.tif", { 42.85,47.62 });
		testRaster(firstReturns / "PointMetricsTest_Mean_CanopyHeight_Meters.tif", 11.);
		testRaster(firstReturns / "PointMetricsTest_StdDev_CanopyHeight_Meters.tif", 5.48);
		testRaster(firstReturns / "PointMetricsTest_TotalReturnCount.tif", 21);

		fs::path frStrata = firstReturns / "StratumMetrics";

		std::vector<metric_t> expectedFRCover = { 100,83.33,45.45,47.62 };
		std::vector<metric_t> expectedFRPercent = { 4.76,23.81,23.81,47.62 };
		for (size_t i = 0; i < strataNames.size(); ++i) {
			testRaster(getStratumName(frStrata, "StratumCover", i), expectedFRCover[i]);
			testRaster(getStratumName(frStrata, "StratumPercent", i), expectedFRPercent[i]);
		}

		//making sure every raster gets tested
		for (auto& f : filesChecked) {
			EXPECT_TRUE(f.second) << "You forgot to write a test for " << f.first;
		}

#pragma warning(pop)
	}
}