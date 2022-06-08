#include"..\LapisObjects.hpp"
#include<gtest/gtest.h>


//The tests in this file are fundamentally about how to interpret user-specified options
//It's likely that some of them will break every time changes are made in those options

namespace lapis {

	namespace fs = std::filesystem;

	//just a wrapper that makes the protected functions of UsableParameters public in order to test them
	class LapisObjectsDerived : public LapisObjects {
	public:
		void identifyLasFiles_test(const FullOptions& opt) {
			identifyLasFiles(opt);
		}
		void identifyDEMFiles_test(const FullOptions& opt) {
			identifyDEMFiles(opt);
		}
		void setFilters_test(const FullOptions& opt) {
			setFilters(opt);
		}
		void createOutAlignment_test(const FullOptions& opt) {
			createOutAlignment(opt);
		}
		void makeNLaz_test() {
			makeNLaz();
		}
	};

	template<class T>
	void verifyNoDuplicates(const std::vector<T>& fileList) {
		std::unordered_set<std::string> uniques;
		for (int i = 0; i < fileList.size(); ++i) {
			uniques.insert(fileList[i].filename);
		}
		EXPECT_EQ(fileList.size(), uniques.size());
	}

	//checks that metric and csm have the right resolution, that they both engulf lasExtent after reprojecting,
	//that metric is a minimal extend around lasExtent, and that csmRes is a minimal extend over lasExtent
	void checkOutAlign(const Alignment& metric, coord_t metricRes,
		const Alignment& csm, coord_t csmRes,
		CoordRef expectedCRS,
		const std::vector<LasFileExtent>& exts) {

		Extent lasExtent;
		bool initExtent = false;
		for (int i = 0; i < exts.size(); ++i) {
			Extent proj = QuadExtent(exts[i].ext, metric.crs()).outerExtent();
			if (!initExtent) {
				lasExtent = proj;
				initExtent = true;
			}
			else {
				lasExtent = extend(lasExtent, proj);
			}
		}

		EXPECT_NEAR(metric.xres(), metricRes, 0.01);
		EXPECT_NEAR(metric.yres(), metricRes, 0.01);
		EXPECT_NEAR(csm.xres(), csmRes, 0.01);
		EXPECT_NEAR(csm.yres(), csmRes, 0.01);

		EXPECT_TRUE(metric.crs().isConsistent(expectedCRS));
		EXPECT_TRUE(csm.crs().isConsistent(expectedCRS));

		if (expectedCRS.getZUnits().status == unitStatus::setByUser) {
			EXPECT_FALSE(metric.crs().getZUnits().isUnknown());
			EXPECT_FALSE(csm.crs().getZUnits().isUnknown());
		}

		Extent projExtent = QuadExtent(lasExtent, metric.crs()).outerExtent();

		EXPECT_TRUE(metric.xmin() <= projExtent.xmin());
		EXPECT_TRUE(csm.xmin() <= projExtent.xmin());

		EXPECT_TRUE(metric.ymin() <= projExtent.ymin());
		EXPECT_TRUE(csm.ymin() <= projExtent.ymin());

		EXPECT_TRUE(metric.xmax() >= projExtent.xmax());
		EXPECT_TRUE(csm.xmax() >= projExtent.xmax());

		EXPECT_TRUE(metric.ymax() >= projExtent.ymax());
		EXPECT_TRUE(csm.ymax() >= projExtent.ymax());

		EXPECT_TRUE(((metric.xmax() - metric.xmin()) - (projExtent.xmax() - projExtent.xmin())) <= 2 * metric.xres());
		EXPECT_TRUE(((metric.ymax() - metric.ymin()) - (projExtent.ymax() - projExtent.ymin())) <= 2 * metric.yres());

		EXPECT_TRUE(((csm.xmax() - csm.xmin()) - (projExtent.xmax() - projExtent.xmin())) <= 2 * csm.xres());
		EXPECT_TRUE(((csm.ymax() - csm.ymin()) - (projExtent.ymax() - projExtent.ymin())) <= 2 * csm.yres());
	}

	//this test uses the laz files in the test files folder as a test--that means that if additional files are added, this test will need to be updated
	TEST(LapisObjectsTest, identifyLasFiles) {
		fs::path testfolder = LAPISTESTFILES;
		const int expectedNumberOfLaz = 5;

		FullOptions opt;
		auto& lazSpec = opt.dataOptions.lasFileSpecifiers;
		LapisObjectsDerived param;
		auto& foundLaz = param.globalProcessingObjects->sortedLasFiles;

		lazSpec = { testfolder.string() };
		param.identifyLasFiles_test(opt);
		EXPECT_EQ(foundLaz.size(), expectedNumberOfLaz);
		verifyNoDuplicates(foundLaz);
		foundLaz.clear();

		//checking recursive seaching
		lazSpec = { (testfolder / "..").string() };
		param.identifyLasFiles_test(opt);
		EXPECT_EQ(foundLaz.size(), expectedNumberOfLaz);
		verifyNoDuplicates(foundLaz);
		foundLaz.clear();

		lazSpec = { (testfolder / "*.laz").string() };
		param.identifyLasFiles_test(opt);
		EXPECT_EQ(foundLaz.size(), expectedNumberOfLaz);
		verifyNoDuplicates(foundLaz);
		foundLaz.clear();

		lazSpec = { (testfolder / "*.las").string() };
		param.identifyLasFiles_test(opt);
		EXPECT_EQ(foundLaz.size(), 0);
		foundLaz.clear();

		lazSpec = { (testfolder / ".." / "*.laz").string() };
		param.identifyLasFiles_test(opt);
		EXPECT_EQ(foundLaz.size(), 0);
		foundLaz.clear();

		lazSpec = { (testfolder / "testlaz10.laz").string(),(testfolder / "testlaz10.laz").string() };
		param.identifyLasFiles_test(opt);
		EXPECT_EQ(foundLaz.size(), 1);
		foundLaz.clear();
	}

	TEST(LapisObjectsTest, identifyDEMFiles) {
		fs::path testfolder = LAPISTESTFILES;
		const int expectedNumberOfDem = 3;

		FullOptions opt;
		auto& demSpec = opt.dataOptions.demFileSpecifiers;
		LapisObjectsDerived param;
		auto& foundDem = param.globalProcessingObjects->demFiles;

		demSpec = { testfolder.string() };
		param.identifyDEMFiles_test(opt);
		EXPECT_EQ(foundDem.size(), expectedNumberOfDem);
		verifyNoDuplicates(foundDem);
		foundDem.clear();

		//checking recursive seaching
		demSpec = { (testfolder / "..").string() };
		param.identifyDEMFiles_test(opt);
		EXPECT_EQ(foundDem.size(), expectedNumberOfDem);
		verifyNoDuplicates(foundDem);
		foundDem.clear();

		demSpec = { (testfolder / "*.img").string() };
		param.identifyDEMFiles_test(opt);
		EXPECT_EQ(foundDem.size(), 2);
		verifyNoDuplicates(foundDem);
		foundDem.clear();

		demSpec = { (testfolder / "*.tif").string() };
		param.identifyDEMFiles_test(opt);
		EXPECT_EQ(foundDem.size(), 1);
		foundDem.clear();

		demSpec = { (testfolder / ".." / "*.img").string() };
		param.identifyDEMFiles_test(opt);
		EXPECT_EQ(foundDem.size(), 0);
		foundDem.clear();

		demSpec = { (testfolder / "testraster.img").string(),(testfolder / "testraster.img").string() };
		param.identifyDEMFiles_test(opt);
		EXPECT_EQ(foundDem.size(), 1);
		foundDem.clear();
	}

	TEST(LapisObjectsTest, setFilters) {
		FullOptions opt;
		LapisObjectsDerived param;
		auto& filters = param.lasProcessingObjects->filters;
		
		opt.processingOptions.useWithheld = false;


		param.setFilters_test(opt);
		EXPECT_EQ(filters.size(), 1); //withheld filter is on by default
		filters.clear();
		
		opt.processingOptions.useWithheld = true;
		param.setFilters_test(opt);
		EXPECT_EQ(filters.size(), 0);
		filters.clear();
		opt.processingOptions.useWithheld = false;

		opt.processingOptions.classes = { true,{1} };
		param.setFilters_test(opt);
		EXPECT_EQ(filters.size(), 2);
		filters.clear();
		opt.processingOptions.classes.reset();

		opt.processingOptions.classes = { false,{1} };
		param.setFilters_test(opt);
		EXPECT_EQ(filters.size(), 2);
		filters.clear();
		opt.processingOptions.classes.reset();

		opt.processingOptions.maxScanAngle = 10;
		param.setFilters_test(opt);
		EXPECT_EQ(filters.size(), 2);
		filters.clear();
		opt.processingOptions.maxScanAngle.reset();

		opt.processingOptions.whichReturns = ProcessingOptions::WhichReturns::all;
		param.setFilters_test(opt);
		EXPECT_EQ(filters.size(), 1);
		filters.clear();

		opt.processingOptions.whichReturns = ProcessingOptions::WhichReturns::first;
		param.setFilters_test(opt);
		EXPECT_EQ(filters.size(), 2);
		filters.clear();
		
		opt.processingOptions.whichReturns = ProcessingOptions::WhichReturns::only;
		param.setFilters_test(opt);
		EXPECT_EQ(filters.size(), 2);
		filters.clear();
	}

	//the code this was testing has been vastly simplified and the tests need to be completely rethought
	/*TEST(LapisObjectsTest, createOutAlignment) {

		CoordRef utm{ "26911" };
		CoordRef sp{ "2927" };
		
		FullOptions opt;
		LapisObjectsDerived param;
		Alignment& outAlign = param.globalProcessingObjects->metricAlign;
		Alignment& csmAlign = param.globalProcessingObjects->csmAlign;

		//first, when the user doesn't specify an alignment from file
		opt.dataOptions.outAlign = ManualAlignment();
		ManualAlignment& ma = std::get<ManualAlignment>(opt.dataOptions.outAlign);
		auto& csmRes = opt.dataOptions.csmRes;


		std::cout << "-----\nspecify nothing\n-----\n";
		param.globalProcessingObjects->sortedLasFiles = { {"a.laz",Extent(0,100,0,100,utm)},
			{"b.laz",Extent(100,200,100,150,utm)} };
		ma.crs = CoordRef("");
		ma.res.reset(); //specifying neither
		csmRes.reset();
		param.createOutAlignment_test(opt);
		checkOutAlign(outAlign, 30,
			csmAlign, 1,
			utm,
			{ {"a.laz",Extent(0,100,0,100,utm)},
			{"b.laz",Extent(100,200,100,150,utm)} });


		std::cout << "-----\nspecify only crs\n-----\n";
		param.globalProcessingObjects->sortedLasFiles = { {"a.laz",Extent(0,100,0,100,utm)},
			{"b.laz",Extent(100,200,100,150,utm)} };
		ma.crs = sp;
		ma.res.reset();
		param.createOutAlignment_test(opt);
		checkOutAlign(outAlign, 98.425,
			csmAlign, 3.2804,
			sp,
			{ {"a.laz",Extent(0,100,0,100,utm)},
			{"b.laz",Extent(100,200,100,150,utm)} });


		std::cout << "-----\nspecify res only\n-----\n";
		param.globalProcessingObjects->sortedLasFiles = { {"a.laz",Extent(0,100,0,100,utm)},
			{"b.laz",Extent(100,200,100,150,utm)} };
		ma.crs = CoordRef("");
		ma.res = 20;
		csmRes = 2;
		opt.dataOptions.outUnits = linearUnitDefs::foot;
		param.createOutAlignment_test(opt);
		CoordRef expCRS = utm;
		expCRS.setZUnits(linearUnitDefs::foot);
		checkOutAlign(outAlign, 6.096,
			csmAlign, 0.6096,
			expCRS,
			{ {"a.laz",Extent(0,100,0,100,utm)},
			{"b.laz",Extent(100,200,100,150,utm)} });

		opt = FullOptions();
		opt.dataOptions.outAlign = AlignmentFromFile();
		auto& aff = std::get<AlignmentFromFile>(opt.dataOptions.outAlign);

		aff.filename = std::string(LAPISTESTFILES) + "testraster.img";
		aff.useType = AlignmentFromFile::alignType::alignOnly;

		std::cout << "-----\nfrom file\n-----\n";
		param.globalProcessingObjects->sortedLasFiles = { {"a.laz",Extent(0,100,0,100,utm)},
			{"b.laz",Extent(100,200,100,150,utm)} };
		param.createOutAlignment_test(opt);
		checkOutAlign(outAlign, 0.75,
			csmAlign, 1,
			utm,
			{ {"a.laz",Extent(0,100,0,100,utm)},
			{"b.laz",Extent(100,200,100,150,utm)} });

		std::cout << "-----\nfrom file, crs mismatch\n-----\n";
		param.globalProcessingObjects->sortedLasFiles = { {"a.laz",Extent(0,100,0,100,sp)},
			{"b.laz",Extent(100,200,100,150,sp)} };
		param.createOutAlignment_test(opt);
		checkOutAlign(outAlign, 0.75,
			csmAlign, 1,
			CoordRef("26911"),
			{ {"a.laz",Extent(0,100,0,100,sp)},
			{"b.laz",Extent(100,200,100,150,sp)} });
	}*/

	TEST(LapisObjectsTest, makeNLaz) {
		LapisObjectsDerived params;
		Alignment& a = params.globalProcessingObjects->metricAlign;
		a = Alignment{ Extent(0,100,200,300),2,2 };

		params.globalProcessingObjects->sortedLasFiles = {
			{"a.laz",Extent(-50,20,150,220)},
			{"b.laz",Extent(0,40,180,320)}
		};
		params.makeNLaz_test();
		std::vector<int> expected = { 1,0,2,0 };

		for (int i = 0; i < a.ncell(); ++i) {
			EXPECT_EQ(expected[i], params.lasProcessingObjects->nLaz[i].value());
		}
	}
}