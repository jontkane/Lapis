#include"test_pch.hpp"
#include"..\parameters\RunParameters.hpp"
#include"..\algorithms\AllDemAlgorithms.hpp"
#include"..\parameters\AllParameters.hpp"
#include"..\algorithms\AllCsmAlgorithms.hpp"
#include"..\algorithms\AllCsmPostProcessors.hpp"
#include"..\algorithms\AllTaoIdAlgorithms.hpp"
#include"..\algorithms\AllTaoSegmentAlgorithms.hpp"


namespace lapis {
	class RunParametersTest : public ::testing::Test {
	public:

		RunParameters& rp() {
			return RunParameters::singleton();
		}

		void TearDown() override {
			rp().resetObject();
		}

		void prepareParams(const std::vector<std::string>& args) {
			rp().resetObject();
			rp().parseArgs(args);
			rp().importBoostAndUpdateUnits();
			rp().prepareForRun();
		}

		void prepareParamsNoSlowStuff(std::vector<std::string> args) {
			args.push_back("--debug-no-alignment");
			args.push_back("--debug-no-output");
			args.push_back("--dem-algo=none");
			prepareParams(args);
		}

		void prepareParamsAllowDems(std::vector<std::string> args) {
			args.push_back("--debug-no-alignment");
			args.push_back("--debug-no-output");
			prepareParams(args);
		}

		template<class T>
		T& getRawParam() {
			return rp().getParam<T>();
		}
	};

	TEST_F(RunParametersTest, demFiles) {
		std::string testFolder = LAPISTESTFILES;

		std::filesystem::remove_all(testFolder + "/output"); //some other tests will write stuff here; they should clean up after themselves but just in case

		auto expectVendorRaster = [&](size_t numberOfRasters) {
			EXPECT_NE(dynamic_cast<VendorRasterApplier<DemParameter>*>(rp().demAlgorithm(LasReader()).get()), nullptr);

			size_t count = 0;
			for (const Alignment& a : getRawParam<DemParameter>().demAligns()) {
				++count;
			}
			EXPECT_EQ(count, numberOfRasters);
		};

		prepareParamsAllowDems({ "--dem=" + testFolder });
		expectVendorRaster(3);

		prepareParamsAllowDems({"--dem=" + testFolder + "/*.img"});
		expectVendorRaster(2);

		prepareParamsAllowDems({"--dem=" + testFolder + "/testraster.img"});
		expectVendorRaster(1);

		prepareParamsAllowDems({ "--dem-algo=none" });
		EXPECT_NE(dynamic_cast<AlreadyNormalizedApplier*>(rp().demAlgorithm(LasReader()).get()), nullptr);
	}

	TEST_F(RunParametersTest, csmOptions) {
		auto checkCsm = [&](coord_t expectedRes) {
			const Alignment& csm = *rp().csmAlign();

			EXPECT_NEAR(csm.xres(), expectedRes,0.0001);
			EXPECT_NEAR(csm.yres(), expectedRes,0.0001);

			const Alignment& a = *rp().metricAlign();
			EXPECT_LE(csm.xmin(), a.xmin());
			EXPECT_GE(csm.xmax(), a.xmax());
			EXPECT_LE(csm.ymin(), a.ymin());
			EXPECT_GE(csm.ymax(), a.ymax());

			EXPECT_NEAR(csm.xOrigin(), std::fmod(a.xOrigin(),csm.xres()), 0.0001);
			EXPECT_NEAR(csm.yOrigin(), std::fmod(a.yOrigin(),csm.yres()), 0.0001);
		};
		
		prepareParamsNoSlowStuff({});
		coord_t defaultRes = rp().csmAlign()->xres();
		checkCsm(defaultRes);
		EXPECT_TRUE(rp().doCsmMetrics());

		prepareParamsNoSlowStuff({"--user-units=ft"});
		checkCsm(convertUnits(defaultRes, linearUnitPresets::meter, linearUnitPresets::internationalFoot));

		prepareParamsNoSlowStuff({"--csm-cellsize=0.5"});
		checkCsm(0.5);

		prepareParamsNoSlowStuff({ "--skip-csm-metrics"});
		EXPECT_FALSE(rp().doCsmMetrics());

		prepareParamsNoSlowStuff({ "--footprint=0.1" });
		MaxPoint* algo = dynamic_cast<MaxPoint*>(rp().csmAlgorithm());
		ASSERT_NE(algo, nullptr);
		EXPECT_EQ(algo->footprintRadius(),0.05);

		prepareParamsNoSlowStuff({ "--smooth=1","--no-csm-fill"});
		EXPECT_NE(dynamic_cast<DoNothingCsm*>(rp().csmPostProcessAlgorithm()), nullptr);

		prepareParamsNoSlowStuff({ "--smooth=3","--no-csm-fill" });
		EXPECT_NE(dynamic_cast<SmoothCsm*>(rp().csmPostProcessAlgorithm()), nullptr);

		prepareParamsNoSlowStuff({ "--smooth=1" });
		EXPECT_NE(dynamic_cast<FillCsm*>(rp().csmPostProcessAlgorithm()), nullptr);

		prepareParamsNoSlowStuff({ "--smooth=3" });
		EXPECT_NE(dynamic_cast<SmoothAndFill*>(rp().csmPostProcessAlgorithm()), nullptr);
	}

	TEST_F(RunParametersTest, filters) {
		bool hasWithheld = false;
		bool hasClassWhite = false;
		bool hasClassBlack = false;
		std::unordered_set<uint8_t> classSet;
		
		prepareParamsNoSlowStuff({ "--use-withheld","--class=3,12,17" });
		coord_t defaultMin = rp().minHt();
		coord_t defaultMax = rp().maxHt();

		auto checkFilters = [&] {
			auto& filters = rp().filters();
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

		EXPECT_NEAR(convertUnits(defaultMin, linearUnitPresets::meter, linearUnitPresets::internationalFoot),rp().minHt(), 0.0001);
		EXPECT_NEAR(convertUnits(defaultMax, linearUnitPresets::meter, linearUnitPresets::internationalFoot), rp().maxHt(), 0.0001);

		prepareParamsNoSlowStuff({ "--minht=200","--maxht=5000" });
		EXPECT_EQ(200, rp().minHt());
		EXPECT_EQ(5000, rp().maxHt());
	}

	TEST_F(RunParametersTest, lasFilesAndAlignment) {

		std::string testFileFolder = LAPISTESTFILES;
		prepareParams({ "--las=" + testFileFolder, "--debug-no-output"});
		const auto& extents = rp().lasExtents();

		EXPECT_EQ(extents.size(), 5);
		CoordRef crs = extents[0].crs();
		for (size_t i = 0; i < extents.size(); ++i) {
			EXPECT_TRUE(extents[i].crs().isConsistent(crs));
		}
		coord_t defaultCellSize = rp().metricAlign()->xres();
		coord_t defaultOrigin = rp().metricAlign()->xOrigin();

		prepareParams({ "--las=" + testFileFolder + "/testlaz10.laz",
			"--las=" + testFileFolder + "/testlaz14.laz", "--las-crs=2286","--las-units=m", "--debug-no-output" });
		const auto& extentstwo = rp().lasExtents();
		EXPECT_EQ(extentstwo.size(), 2);
		crs = CoordRef("2286", linearUnitPresets::meter);
		for (size_t i = 0; i < extentstwo.size(); ++i) {
			EXPECT_TRUE(extentstwo[i].crs().isConsistentHoriz(crs));
			EXPECT_TRUE(extentstwo[i].crs().isConsistentZUnits(crs));
		}

		Alignment metricAlign = *rp().metricAlign();
		EXPECT_TRUE(metricAlign.crs().isConsistent(crs));
		for (size_t i = 0; i < extentstwo.size(); ++i) {
			EXPECT_GE(extentstwo[i].xmin(), metricAlign.xmin());
			EXPECT_GE(extentstwo[i].ymin(), metricAlign.ymin());
			EXPECT_LE(extentstwo[i].xmax(), metricAlign.xmax());
			EXPECT_LE(extentstwo[i].ymax(), metricAlign.ymax());
		}

		auto roundOriginToZero = [](coord_t origin, coord_t res) {
			return std::min(origin, std::abs(res - origin));
		};

		EXPECT_NEAR(convertUnits(defaultCellSize, linearUnitPresets::meter, crs.getXYUnits()), metricAlign.xres(),0.01);
		EXPECT_NEAR(convertUnits(defaultCellSize, linearUnitPresets::meter, crs.getXYUnits()), metricAlign.yres(), 0.01);
		EXPECT_NEAR(convertUnits(defaultOrigin, linearUnitPresets::meter, crs.getXYUnits()), roundOriginToZero(metricAlign.xOrigin(),metricAlign.xres()), 0.01);
		EXPECT_NEAR(convertUnits(defaultOrigin, linearUnitPresets::meter, crs.getXYUnits()), roundOriginToZero(metricAlign.yOrigin(),metricAlign.yres()), 0.01);

		Alignment fileAlign{ testFileFolder + "/testRaster.tif" };
		prepareParams({ "--las=" + testFileFolder + "/testlaz10.laz",
			"--alignment=" + testFileFolder + "/testRaster.tif", "--debug-no-output" });
		metricAlign = *rp().metricAlign();
		EXPECT_TRUE(fileAlign.crs().isConsistent(metricAlign.crs()));
		EXPECT_NEAR(fileAlign.xres(), metricAlign.xres(), 0.000001);
		EXPECT_NEAR(fileAlign.yres(), metricAlign.yres(), 0.000001);
		EXPECT_NEAR(fileAlign.xOrigin(), roundOriginToZero(metricAlign.xOrigin(),metricAlign.xres()), 0.0001);
		EXPECT_NEAR(fileAlign.yOrigin(), roundOriginToZero(metricAlign.yOrigin(),metricAlign.yres()), 0.0001);


		prepareParams({ "--las=" + testFileFolder + "/testlaz10.laz",
			"--out-crs=2286", "--user-units=m","--cellsize=20","--xorigin=10","--yorigin=5", "--debug-no-output" });
		metricAlign = *rp().metricAlign();
		EXPECT_TRUE(metricAlign.crs().isConsistentHoriz(CoordRef("2286")));
		EXPECT_NEAR(convertUnits(20, linearUnitPresets::meter, metricAlign.crs().getXYUnits()), metricAlign.xres(), 0.0001);
		EXPECT_NEAR(convertUnits(20, linearUnitPresets::meter, metricAlign.crs().getXYUnits()), metricAlign.yres(), 0.0001);
		EXPECT_NEAR(convertUnits(10, linearUnitPresets::meter, metricAlign.crs().getXYUnits()), metricAlign.xOrigin(), 0.0001);
		EXPECT_NEAR(convertUnits(5, linearUnitPresets::meter, metricAlign.crs().getXYUnits()), metricAlign.yOrigin(), 0.0001);

		prepareParams({ "--las=" + testFileFolder + "/testlaz10.laz", "--out-crs=2286", "--user-units=m","--yres=9","--xres=7", "--debug-no-output" });
		metricAlign = *rp().metricAlign();
		EXPECT_NEAR(convertUnits(7, linearUnitPresets::meter, metricAlign.crs().getXYUnits()), metricAlign.xres(), 0.0001);
		EXPECT_NEAR(convertUnits(9, linearUnitPresets::meter, metricAlign.crs().getXYUnits()), metricAlign.yres(), 0.0001);
	}

	TEST_F(RunParametersTest, computerOptions) {
		prepareParamsNoSlowStuff({ "--thread=129" });
		EXPECT_EQ(129, rp().nThread());
		coord_t noPerfBinSize = rp().binSize();
	}

	TEST_F(RunParametersTest, pointMetricOptions) {
		prepareParamsNoSlowStuff({});
		//defaults might change so I just want to test that they aren't default-constructed
		coord_t defaultCanopy = rp().canopyCutoff();
		std::vector<coord_t> defaultStrata = rp().strataBreaks();
		EXPECT_GT(defaultCanopy, 0);
		EXPECT_GT(defaultStrata.size(), 0);
		for (size_t i = 1; i < defaultStrata.size(); ++i) {
			EXPECT_GT(defaultStrata[i], defaultStrata[i - 1]);
		}
		EXPECT_TRUE(rp().doStratumMetrics());

		prepareParamsNoSlowStuff({"--user-units=ft"});
		coord_t footCanopy = rp().canopyCutoff();
		std::vector<coord_t> footStrata = rp().strataBreaks();
		EXPECT_NEAR(defaultCanopy, footCanopy*0.3048,0.01);
		EXPECT_EQ(footStrata.size(), defaultStrata.size());
		for (size_t i = 0; i < defaultStrata.size(); ++i) {
			EXPECT_NEAR(defaultStrata[i], footStrata[i] * 0.3048, 0.01);
		}

		prepareParamsNoSlowStuff({"--canopy=5","--strata=0,1,2,3"});
		coord_t newCanopy = rp().canopyCutoff();
		std::vector<coord_t> newStrata = rp().strataBreaks();
		EXPECT_EQ(newCanopy, 5);
		EXPECT_EQ(newStrata.size(), 4);
		for (size_t i = 0; i < newStrata.size(); ++i) {
			EXPECT_EQ(newStrata[i], i);
		}

		prepareParamsNoSlowStuff({ "--skip-strata" });
		EXPECT_EQ(rp().strataBreaks().size(), 0);
		EXPECT_EQ(rp().strataNames().size(), 0);
		EXPECT_FALSE(rp().doStratumMetrics());
	}

	TEST_F(RunParametersTest, name) {
		std::string testName = "name";
		prepareParamsNoSlowStuff({ "-N", testName });
		EXPECT_EQ(testName, rp().name());
	}

	TEST_F(RunParametersTest, output) {
		std::string testOutput = "C:/";
		prepareParams({ "-O", testOutput,"--debug-no-alignment"});
		EXPECT_EQ(testOutput, rp().outFolder());
	}

	TEST_F(RunParametersTest, whichProducts) {
		prepareParamsNoSlowStuff({});
		EXPECT_TRUE(rp().doCsm());
		EXPECT_FALSE(rp().doFineInt());
		EXPECT_TRUE(rp().doPointMetrics());
		EXPECT_TRUE(rp().doTaos());
		EXPECT_TRUE(rp().doTopo());

		prepareParamsNoSlowStuff({"--skip-csm"});
		EXPECT_FALSE(rp().doCsm());
		EXPECT_FALSE(rp().doFineInt());
		EXPECT_TRUE(rp().doPointMetrics());
		EXPECT_FALSE(rp().doTaos());
		EXPECT_TRUE(rp().doTopo());

		prepareParamsNoSlowStuff({ "--fine-int" });
		EXPECT_TRUE(rp().doCsm());
		EXPECT_TRUE(rp().doFineInt());
		EXPECT_TRUE(rp().doPointMetrics());
		EXPECT_TRUE(rp().doTaos());
		EXPECT_TRUE(rp().doTopo());

		prepareParamsNoSlowStuff({"--skip-point-metrics"});
		EXPECT_TRUE(rp().doCsm());
		EXPECT_FALSE(rp().doFineInt());
		EXPECT_FALSE(rp().doPointMetrics());
		EXPECT_TRUE(rp().doTaos());
		EXPECT_TRUE(rp().doTopo());

		prepareParamsNoSlowStuff({"--skip-tao"});
		EXPECT_TRUE(rp().doCsm());
		EXPECT_FALSE(rp().doFineInt());
		EXPECT_TRUE(rp().doPointMetrics());
		EXPECT_FALSE(rp().doTaos());
		EXPECT_TRUE(rp().doTopo());

		prepareParamsNoSlowStuff({"--skip-topo"});
		EXPECT_TRUE(rp().doCsm());
		EXPECT_FALSE(rp().doFineInt());
		EXPECT_TRUE(rp().doPointMetrics());
		EXPECT_TRUE(rp().doTaos());
		EXPECT_FALSE(rp().doTopo());
	}

	TEST_F(RunParametersTest, taoOptions) {

		auto expectHighPoints = [&](coord_t minHt, coord_t minDist) {
			HighPoints* algo = dynamic_cast<HighPoints*>(rp().taoIdAlgorithm());
			ASSERT_NE(algo, nullptr);
			EXPECT_EQ(algo->minDist(), minDist);
			EXPECT_EQ(algo->minHt(), minHt);
		};

		prepareParamsNoSlowStuff({});
		expectHighPoints(rp().canopyCutoff(), 0);

		prepareParamsNoSlowStuff({ "--canopy=5" });
		expectHighPoints(5, 0);

		prepareParamsNoSlowStuff({ "--canopy=3","--min-tao-ht=4" });
		expectHighPoints(4,0);

		prepareParamsNoSlowStuff({ "--min-tao-dist=2" });
		expectHighPoints(rp().canopyCutoff(), 2);
	}
	
	TEST_F(RunParametersTest, fineIntOptions) {
		auto checkAlign = [&](coord_t expectedRes) {
			const Alignment& fineInt = *rp().fineIntAlign();

			EXPECT_NEAR(fineInt.xres(), expectedRes, 0.0001);
			EXPECT_NEAR(fineInt.yres(), expectedRes, 0.0001);

			const Alignment& a = *rp().metricAlign();
			EXPECT_LE(fineInt.xmin(), a.xmin());
			EXPECT_GE(fineInt.xmax(), a.xmax());
			EXPECT_LE(fineInt.ymin(), a.ymin());
			EXPECT_GE(fineInt.ymax(), a.ymax());

			EXPECT_NEAR(fineInt.xOrigin(), std::fmod(a.xOrigin(), fineInt.xres()), 0.0001);
			EXPECT_NEAR(fineInt.yOrigin(), std::fmod(a.yOrigin(), fineInt.yres()), 0.0001);
		};

		prepareParamsNoSlowStuff({"--fine-int"});
		checkAlign(rp().csmAlign()->xres());

		prepareParamsNoSlowStuff({"--fine-int", "--csm-cellsize=5"});
		checkAlign(rp().csmAlign()->xres());

		prepareParamsNoSlowStuff({ "--fine-int","--csm-cellsize=5","--fine-int-cellsize=20"});
		checkAlign(20);


		prepareParamsNoSlowStuff({});
		EXPECT_EQ(rp().fineIntCanopyCutoff(), rp().canopyCutoff());

		prepareParamsNoSlowStuff({ "--fine-int","--canopy=10" });
		EXPECT_EQ(rp().fineIntCanopyCutoff(), rp().canopyCutoff());

		prepareParamsNoSlowStuff({ "--fine-int","--canopy=11","--fine-int-cutoff=7" });
		EXPECT_EQ(rp().fineIntCanopyCutoff(), 7);

		prepareParamsNoSlowStuff({ "--fine-int","--fine-int-no-cutoff" });
		EXPECT_EQ(rp().fineIntCanopyCutoff(), std::numeric_limits<coord_t>::lowest());
	}
}