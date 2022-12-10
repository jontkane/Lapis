#include"..\LapisData.hpp"
#include<gtest/gtest.h>

namespace lapis {
	class LapisDataTest : public ::testing::Test {
	public:

		LapisData& data() {
			return LapisData::getDataObject();
		}

		void TearDown() override {
			data().resetObject();
		}

		void prepareParams(const std::vector<std::string>& args) {
			data().resetObject();
			data().parseArgs(args);
			data().importBoostAndUpdateUnits();
			data().prepareForRun();
		}

		void prepareParamsNoSlowStuff(std::vector<std::string> args) {
			args.push_back("--debug-no-alignment");
			args.push_back("--debug-no-alloc-raster");
			args.push_back("--debug-no-output");
			prepareParams(args);
		}
	};

	TEST_F(LapisDataTest, demFiles) {
		std::string testFolder = LAPISTESTFILES;
		prepareParamsNoSlowStuff({ "--dem=" + testFolder });
		EXPECT_EQ(data().demList().size(), 3);

		prepareParamsNoSlowStuff({"--dem=" + testFolder + "/*.img"});
		EXPECT_EQ(data().demList().size(), 2);

		prepareParamsNoSlowStuff({"--dem=" + testFolder + "/testraster.img"});
		EXPECT_EQ(data().demList().size(), 1);
	}

	TEST_F(LapisDataTest, csmOptions) {
		auto checkCsm = [&](coord_t expectedRes) {
			const Alignment& csm = *data().csmAlign();

			EXPECT_NEAR(csm.xres(), expectedRes,0.0001);
			EXPECT_NEAR(csm.yres(), expectedRes,0.0001);

			const Alignment& a = *data().metricAlign();
			EXPECT_LE(csm.xmin(), a.xmin());
			EXPECT_GE(csm.xmax(), a.xmax());
			EXPECT_LE(csm.ymin(), a.ymin());
			EXPECT_GE(csm.ymax(), a.ymax());

			EXPECT_NEAR(csm.xOrigin(), std::fmod(a.xOrigin(),csm.xres()), 0.0001);
			EXPECT_NEAR(csm.yOrigin(), std::fmod(a.yOrigin(),csm.yres()), 0.0001);
		};
		
		prepareParamsNoSlowStuff({});
		coord_t defaultRes = data().csmAlign()->xres();
		checkCsm(defaultRes);

		prepareParamsNoSlowStuff({"--user-units=ft"});
		checkCsm(convertUnits(defaultRes, LinearUnitDefs::meter, LinearUnitDefs::foot));

		prepareParamsNoSlowStuff({"--csm-cellsize=0.5"});
		checkCsm(0.5);

		prepareParams({ "--debug-no-alignment","--debug-no-output","--skip-csm-metrics"});
		EXPECT_EQ(data().csmMetrics().size(), 0);

		prepareParamsNoSlowStuff({ "--footprint=2" });
		EXPECT_EQ(data().footprintDiameter(), 2);

		prepareParamsNoSlowStuff({ "--smooth=5" });
		EXPECT_EQ(data().smooth(), 5);
	}

	TEST_F(LapisDataTest, nLazRaster) {
		std::string testFolder = LAPISTESTFILES;
		prepareParams({ "--debug-no-alloc-raster", "--las=" + testFolder + "/testlaz14.laz", "--cellsize=0.1","--debug-no-output"});
		Raster<int> r1 = *data().nLazRaster();
		EXPECT_EQ(*data().metricAlign(), (Alignment)r1);
		bool anyHasValue = false;
		for (cell_t cell = 0; cell < r1.ncell(); ++cell) {
			if (r1[cell].has_value()) {
				EXPECT_EQ(r1[cell].value(), 1);
				anyHasValue = true;
			}
		}
		EXPECT_TRUE(anyHasValue);

		prepareParams({ "--debug-no-alloc-raster","--las=" + testFolder + "/testlaz14.laz",
			"--las=" + testFolder + "/testlaznormalized.laz","--cellsize=0.1","--debug-no-output"});
		Raster<int> r2 = *data().nLazRaster();
		EXPECT_EQ((Alignment)r1, Alignment(r2));
		for (cell_t cell = 0; cell < r2.ncell(); ++cell) {
			if (r1[cell].has_value()) {
				EXPECT_TRUE(r2[cell].has_value());
				EXPECT_EQ(r2[cell].value(), 2);
			}
			else {
				EXPECT_FALSE(r2[cell].has_value());
			}
		}
	}

	TEST_F(LapisDataTest, metricRasters) {
		std::string debugParam = "--debug-no-alignment";
		prepareParams({ debugParam, "--strata=1,2,3","--debug-no-output"});

		const Alignment& a = *data().metricAlign();
		
		size_t metricN = data().allReturnPointMetrics().size();
		
		auto& arpm = data().allReturnPointMetrics();
		EXPECT_GT(arpm.size(), 0);
		for (auto& m : arpm) {
			EXPECT_EQ(a, (Alignment)m.rast);
		}
		auto& frpm = data().firstReturnPointMetrics();
		EXPECT_EQ(frpm.size(), arpm.size());
		for (auto& m : frpm) {
			EXPECT_EQ(a, (Alignment)m.rast);
		}
		auto& arsm = data().allReturnStratumMetrics();
		EXPECT_GT(arsm.size(), 0);
		for (auto& m : arsm) {
			EXPECT_EQ(m.rasters.size(), 4);
			for (auto& r : m.rasters) {
				EXPECT_EQ(a, (Alignment)r);
			}
		}
		auto& frsm = data().firstReturnStratumMetrics();
		EXPECT_EQ(arsm.size(), frsm.size());
		for (auto& m : frsm) {
			EXPECT_EQ(m.rasters.size(), 4);
			for (auto& r : m.rasters) {
				EXPECT_EQ(a, (Alignment)r);
			}
		}
		auto& csm = data().csmMetrics();
		EXPECT_GT(csm.size(), 0);
		for (auto& m : csm) {
			EXPECT_EQ(a, (Alignment)m.r);
		}
		auto& topo = data().topoMetrics();
		EXPECT_GT(topo.size(), 0);

		auto& elevnum = *data().elevNum();
		EXPECT_EQ(a, (Alignment)elevnum);
		auto& elevdenom = *data().elevDenom();
		EXPECT_EQ(a, (Alignment)elevdenom);

		auto& arpmc = *data().allReturnPMC();
		EXPECT_EQ(a, (Alignment)arpmc);
		auto& frpmc = *data().firstReturnPMC();
		EXPECT_EQ(a, (Alignment)frpmc);
	}

	TEST_F(LapisDataTest, filters) {
		bool hasWithheld = false;
		bool hasClassWhite = false;
		bool hasClassBlack = false;
		std::unordered_set<uint8_t> classSet;
		
		prepareParamsNoSlowStuff({ "--use-withheld","--class=3,12,17" });
		coord_t defaultMin = data().minHt();
		coord_t defaultMax = data().maxHt();

		auto checkFilters = [&] {
			auto& filters = data().filters();
			hasWithheld = false;
			hasClassBlack = false;
			hasClassWhite = false;
			for (size_t i = 0; i < filters.size(); ++i) {
				if (dynamic_cast<LasFilterWithheld*>(filters[i].get())) {
					hasWithheld = true;
				}
				if (dynamic_cast<LasFilterClassBlacklist*>(filters[i].get())) {
					hasClassBlack = true;
					auto ptr = dynamic_cast<LasFilterClassBlacklist*>(filters[i].get());
					classSet = ptr->getSet();
				}
				if (dynamic_cast<LasFilterClassWhitelist*>(filters[i].get())) {
					hasClassWhite = true;
					auto ptr = dynamic_cast<LasFilterClassWhitelist*>(filters[i].get());
					classSet = ptr->getSet();
				}
			}
		};

		checkFilters();
		EXPECT_TRUE(hasClassWhite);
		EXPECT_FALSE(hasClassBlack);
		EXPECT_FALSE(hasWithheld);
		EXPECT_EQ(classSet.size(), 3);
		EXPECT_TRUE(classSet.contains(3));
		EXPECT_TRUE(classSet.contains(12));
		EXPECT_TRUE(classSet.contains(17));

		prepareParamsNoSlowStuff({ "--class=~3,12,17","--user-units=ft"});
		checkFilters();
		EXPECT_TRUE(hasClassBlack);
		EXPECT_TRUE(hasWithheld);
		EXPECT_FALSE(hasClassWhite);
		EXPECT_EQ(classSet.size(), 3);
		EXPECT_TRUE(classSet.contains(3));
		EXPECT_TRUE(classSet.contains(12));
		EXPECT_TRUE(classSet.contains(17));

		EXPECT_NEAR(convertUnits(defaultMin, LinearUnitDefs::meter, LinearUnitDefs::foot),data().minHt(), 0.0001);
		EXPECT_NEAR(convertUnits(defaultMax, LinearUnitDefs::meter, LinearUnitDefs::foot), data().maxHt(), 0.0001);

		prepareParamsNoSlowStuff({ "--minht=200","--maxht=5000" });
		EXPECT_EQ(200, data().minHt());
		EXPECT_EQ(5000, data().maxHt());
	}

	TEST_F(LapisDataTest, lasFilesAndAlignment) {
		std::string debugParam = "--debug-no-alloc-raster";

		std::string testFileFolder = LAPISTESTFILES;
		prepareParams({ debugParam, "--las=" + testFileFolder, "--debug-no-output"});
		const auto& files = data().sortedLasList();

		EXPECT_EQ(files.size(), 5);
		CoordRef crs = files[0].ext.crs();
		for (size_t i = 0; i < files.size(); ++i) {
			EXPECT_TRUE(files[i].ext.crs().isConsistent(crs));
		}
		coord_t defaultCellSize = data().metricAlign()->xres();
		coord_t defaultOrigin = data().metricAlign()->xOrigin();

		prepareParams({ debugParam, "--las=" + testFileFolder + "/testlaz10.laz",
			"--las=" + testFileFolder + "/testlaz14.laz", "--las-crs=2286","--las-units=m", "--debug-no-output" });
		const auto& filestwo = data().sortedLasList();
		EXPECT_EQ(filestwo.size(), 2);
		crs = CoordRef("2286", LinearUnitDefs::meter);
		for (size_t i = 0; i < filestwo.size(); ++i) {
			EXPECT_TRUE(filestwo[i].ext.crs().isConsistentHoriz(crs));
			EXPECT_TRUE(filestwo[i].ext.crs().isConsistentZUnits(crs));
		}

		Alignment metricAlign = *data().metricAlign();
		EXPECT_TRUE(metricAlign.crs().isConsistent(crs));
		for (size_t i = 0; i < filestwo.size(); ++i) {
			EXPECT_GE(filestwo[i].ext.xmin(), metricAlign.xmin());
			EXPECT_GE(filestwo[i].ext.ymin(), metricAlign.ymin());
			EXPECT_LE(filestwo[i].ext.xmax(), metricAlign.xmax());
			EXPECT_LE(filestwo[i].ext.ymax(), metricAlign.ymax());
		}

		auto roundOriginToZero = [](coord_t origin, coord_t res) {
			return std::min(origin, std::abs(res - origin));
		};

		EXPECT_NEAR(convertUnits(defaultCellSize, LinearUnitDefs::meter, crs.getXYUnits()), metricAlign.xres(),0.01);
		EXPECT_NEAR(convertUnits(defaultCellSize, LinearUnitDefs::meter, crs.getXYUnits()), metricAlign.yres(), 0.01);
		EXPECT_NEAR(convertUnits(defaultOrigin, LinearUnitDefs::meter, crs.getXYUnits()), roundOriginToZero(metricAlign.xOrigin(),metricAlign.xres()), 0.01);
		EXPECT_NEAR(convertUnits(defaultOrigin, LinearUnitDefs::meter, crs.getXYUnits()), roundOriginToZero(metricAlign.yOrigin(),metricAlign.yres()), 0.01);

		Alignment fileAlign{ testFileFolder + "/testRaster.tif" };
		prepareParams({ debugParam, "--las=" + testFileFolder + "/testlaz10.laz",
			"--alignment=" + testFileFolder + "/testRaster.tif", "--debug-no-output" });
		metricAlign = *data().metricAlign();
		EXPECT_TRUE(fileAlign.crs().isConsistent(metricAlign.crs()));
		EXPECT_NEAR(fileAlign.xres(), metricAlign.xres(), 0.000001);
		EXPECT_NEAR(fileAlign.yres(), metricAlign.yres(), 0.000001);
		EXPECT_NEAR(fileAlign.xOrigin(), roundOriginToZero(metricAlign.xOrigin(),metricAlign.xres()), 0.0001);
		EXPECT_NEAR(fileAlign.yOrigin(), roundOriginToZero(metricAlign.yOrigin(),metricAlign.yres()), 0.0001);


		prepareParams({ debugParam, "--las=" + testFileFolder + "/testlaz10.laz",
			"--out-crs=2286", "--user-units=m","--cellsize=20","--xorigin=10","--yorigin=5", "--debug-no-output" });
		metricAlign = *data().metricAlign();
		EXPECT_TRUE(metricAlign.crs().isConsistentHoriz(CoordRef("2286")));
		EXPECT_NEAR(convertUnits(20, LinearUnitDefs::meter, metricAlign.crs().getXYUnits()), metricAlign.xres(), 0.0001);
		EXPECT_NEAR(convertUnits(20, LinearUnitDefs::meter, metricAlign.crs().getXYUnits()), metricAlign.yres(), 0.0001);
		EXPECT_NEAR(convertUnits(10, LinearUnitDefs::meter, metricAlign.crs().getXYUnits()), metricAlign.xOrigin(), 0.0001);
		EXPECT_NEAR(convertUnits(5, LinearUnitDefs::meter, metricAlign.crs().getXYUnits()), metricAlign.yOrigin(), 0.0001);

		prepareParams({ debugParam, "--las=" + testFileFolder + "/testlaz10.laz", "--out-crs=2286", "--user-units=m","--yres=9","--xres=7", "--debug-no-output" });
		metricAlign = *data().metricAlign();
		EXPECT_NEAR(convertUnits(7, LinearUnitDefs::meter, metricAlign.crs().getXYUnits()), metricAlign.xres(), 0.0001);
		EXPECT_NEAR(convertUnits(9, LinearUnitDefs::meter, metricAlign.crs().getXYUnits()), metricAlign.yres(), 0.0001);
	}

	TEST_F(LapisDataTest, computerOptions) {
		prepareParamsNoSlowStuff({ "--thread=129" });
		EXPECT_EQ(129, data().nThread());
		coord_t noPerfBinSize = data().binSize();
	}

	TEST_F(LapisDataTest, pointMetricOptions) {
		prepareParamsNoSlowStuff({});
		//defaults might change so I just want to test that they aren't default-constructed
		coord_t defaultCanopy = data().canopyCutoff();
		std::vector<coord_t> defaultStrata = data().strataBreaks();
		EXPECT_GT(defaultCanopy, 0);
		EXPECT_GT(defaultStrata.size(), 0);
		for (size_t i = 1; i < defaultStrata.size(); ++i) {
			EXPECT_GT(defaultStrata[i], defaultStrata[i - 1]);
		}

		prepareParamsNoSlowStuff({"--user-units=ft"});
		coord_t footCanopy = data().canopyCutoff();
		std::vector<coord_t> footStrata = data().strataBreaks();
		EXPECT_NEAR(defaultCanopy, footCanopy*0.3048,0.01);
		EXPECT_EQ(footStrata.size(), defaultStrata.size());
		for (size_t i = 0; i < defaultStrata.size(); ++i) {
			EXPECT_NEAR(defaultStrata[i], footStrata[i] * 0.3048, 0.01);
		}

		prepareParamsNoSlowStuff({"--canopy=5","--strata=0,1,2,3"});
		coord_t newCanopy = data().canopyCutoff();
		std::vector<coord_t> newStrata = data().strataBreaks();
		EXPECT_EQ(newCanopy, 5);
		EXPECT_EQ(newStrata.size(), 4);
		for (size_t i = 0; i < newStrata.size(); ++i) {
			EXPECT_EQ(newStrata[i], i);
		}

		prepareParamsNoSlowStuff({ "--skip-strata" });
		EXPECT_EQ(data().strataBreaks().size(), 0);

		size_t metricCount = data().allReturnPointMetrics().size();

		prepareParams({ "--debug-no-alignment","--debug-no-output","--skip-all-returns","--adv-point" });
		EXPECT_EQ(data().allReturnPointMetrics().size(), 0);
		EXPECT_GT(data().firstReturnPointMetrics().size(), metricCount);
		EXPECT_EQ(data().allReturnStratumMetrics().size(), 0);
		EXPECT_GT(data().firstReturnStratumMetrics().size(), 0);

		prepareParams({ "--debug-no-alignment","--debug-no-output","--skip-first-returns","--adv-point","--skip-strata"});
		EXPECT_EQ(data().firstReturnPointMetrics().size(), 0);
		EXPECT_GT(data().allReturnPointMetrics().size(), metricCount);
		EXPECT_EQ(data().allReturnStratumMetrics().size(), 0);
		EXPECT_EQ(data().firstReturnStratumMetrics().size(), 0);
	}

	TEST_F(LapisDataTest, name) {
		std::string testName = "name";
		prepareParamsNoSlowStuff({ "-N", testName });
		EXPECT_EQ(testName, data().name());
	}

	TEST_F(LapisDataTest, output) {
		std::string testOutput = "C:/";
		prepareParams({ "-O", testOutput,"--debug-no-alloc-raster","--debug-no-alignment"});
		EXPECT_EQ(testOutput, data().outFolder());
	}

	TEST_F(LapisDataTest, whichProducts) {
		prepareParamsNoSlowStuff({});
		EXPECT_TRUE(data().doCsm());
		EXPECT_FALSE(data().doFineInt());
		EXPECT_TRUE(data().doPointMetrics());
		EXPECT_TRUE(data().doTaos());
		EXPECT_TRUE(data().doTopo());

		prepareParamsNoSlowStuff({"--skip-csm"});
		EXPECT_FALSE(data().doCsm());
		EXPECT_FALSE(data().doFineInt());
		EXPECT_TRUE(data().doPointMetrics());
		EXPECT_FALSE(data().doTaos());
		EXPECT_TRUE(data().doTopo());

		prepareParamsNoSlowStuff({ "--fine-int" });
		EXPECT_TRUE(data().doCsm());
		EXPECT_TRUE(data().doFineInt());
		EXPECT_TRUE(data().doPointMetrics());
		EXPECT_TRUE(data().doTaos());
		EXPECT_TRUE(data().doTopo());

		prepareParamsNoSlowStuff({"--skip-point-metrics"});
		EXPECT_TRUE(data().doCsm());
		EXPECT_FALSE(data().doFineInt());
		EXPECT_FALSE(data().doPointMetrics());
		EXPECT_TRUE(data().doTaos());
		EXPECT_TRUE(data().doTopo());

		prepareParamsNoSlowStuff({"--skip-tao"});
		EXPECT_TRUE(data().doCsm());
		EXPECT_FALSE(data().doFineInt());
		EXPECT_TRUE(data().doPointMetrics());
		EXPECT_FALSE(data().doTaos());
		EXPECT_TRUE(data().doTopo());

		prepareParamsNoSlowStuff({"--skip-topo"});
		EXPECT_TRUE(data().doCsm());
		EXPECT_FALSE(data().doFineInt());
		EXPECT_TRUE(data().doPointMetrics());
		EXPECT_TRUE(data().doTaos());
		EXPECT_FALSE(data().doTopo());
	}

	TEST_F(LapisDataTest, taoOptions) {
		prepareParamsNoSlowStuff({});
		EXPECT_EQ(data().minTaoDist(), 0);
		EXPECT_EQ(data().minTaoHt(), data().canopyCutoff());

		prepareParamsNoSlowStuff({ "--canopy=5" });
		EXPECT_EQ(data().minTaoHt(), 5);

		prepareParamsNoSlowStuff({ "--canopy=3","--min-tao-ht=4" });
		EXPECT_EQ(data().minTaoHt(), 4);

		prepareParamsNoSlowStuff({ "--min-tao-dist=2" });
		EXPECT_EQ(data().minTaoDist(), 2);
	}
	
	TEST_F(LapisDataTest, fineIntOptions) {
		auto checkAlign = [&](coord_t expectedRes) {
			const Alignment& fineInt = *data().fineIntAlign();

			EXPECT_NEAR(fineInt.xres(), expectedRes, 0.0001);
			EXPECT_NEAR(fineInt.yres(), expectedRes, 0.0001);

			const Alignment& a = *data().metricAlign();
			EXPECT_LE(fineInt.xmin(), a.xmin());
			EXPECT_GE(fineInt.xmax(), a.xmax());
			EXPECT_LE(fineInt.ymin(), a.ymin());
			EXPECT_GE(fineInt.ymax(), a.ymax());

			EXPECT_NEAR(fineInt.xOrigin(), std::fmod(a.xOrigin(), fineInt.xres()), 0.0001);
			EXPECT_NEAR(fineInt.yOrigin(), std::fmod(a.yOrigin(), fineInt.yres()), 0.0001);
		};

		prepareParamsNoSlowStuff({"--fine-int"});
		checkAlign(data().csmAlign()->xres());

		prepareParamsNoSlowStuff({"--fine-int", "--csm-cellsize=5"});
		checkAlign(data().csmAlign()->xres());

		prepareParamsNoSlowStuff({ "--fine-int","--csm-cellsize=5","--fine-int-cellsize=20"});
		checkAlign(20);


		prepareParamsNoSlowStuff({});
		EXPECT_EQ(data().fineIntCanopyCutoff(), data().canopyCutoff());

		prepareParamsNoSlowStuff({ "--fine-int","--canopy=10" });
		EXPECT_EQ(data().fineIntCanopyCutoff(), data().canopyCutoff());

		prepareParamsNoSlowStuff({ "--fine-int","--canopy=11","--fine-int-cutoff=7" });
		EXPECT_EQ(data().fineIntCanopyCutoff(), 7);

		prepareParamsNoSlowStuff({ "--fine-int","--fine-int-no-cutoff" });
		EXPECT_EQ(data().fineIntCanopyCutoff(), std::numeric_limits<coord_t>::lowest());
	}
}