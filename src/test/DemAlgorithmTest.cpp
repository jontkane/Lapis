#include"test_pch.hpp"
#include"..\algorithms\AllDemAlgorithms.hpp"

namespace lapis {

	class DemSpoofer {
	public:
		void addDem(const Raster<csm_t>& r) {
			_aligns.push_back(r);

			_rasters.push_back(r);
		}

		Raster<coord_t> getDem(size_t n, const Extent& e) { return _rasters[n]; }
		auto& demAligns() { return _aligns;	}
		const Alignment& demAlign(size_t index, const CoordRef& crs) { return _aligns[index]; }
		size_t nDem() const { return _aligns.size(); }

	private:
		std::vector<Alignment> _aligns;
		std::vector<Raster<coord_t>> _rasters;
	};

	TEST(DemAlgoTest, vendorrastertest) {

		CoordRef epsg2927{ "2927",linearUnitPresets::usSurveyFoot };
		CoordRef epsg2285{ "2285",linearUnitPresets::usSurveyFoot };

		Raster<coord_t> demOne{ Alignment(Extent(0,5,0,5,epsg2927),5,5)};
		for (cell_t cell = 0; cell < demOne.ncell(); ++cell) {
			demOne[cell].has_value() = true;
			demOne[cell].value() = 1;
		}
		Raster<coord_t> demTwo{ Alignment(Extent(0,10,0,10,epsg2927),2,2)};
		for (cell_t cell = 0; cell < demTwo.ncell(); ++cell) {
			demTwo[cell].has_value() = true;
			demTwo[cell].value() = 10;
		}

		DemSpoofer spoof;
		spoof.addDem(demOne);
		spoof.addDem(demTwo);


		VendorRaster<DemSpoofer> algo(&spoof);
		algo.setMinMax(0, 100);

		auto freshPoints = [&]()->LidarPointVector {
			LidarPointVector lpv{ epsg2927 };
			lpv.push_back({ 0.5,0.5,11,0,0 }); //normalizes to 10 in the fine raster and 1 in the coarse raster. expected: 10
			lpv.push_back({ 7.5,7.5,11,0,0 }); //can't be normalized by the fine raster, normalizes to 1 in the coarse raster. expected: 1
			lpv.push_back({ 0.5,0.5,102,0,0 }); //normalizes to 101 in the fine raster and 92 in the coarse raster. expected: filtered
			lpv.push_back({ 0.5,0.5,500,0,0 }); //far too high. expected:filtered
			lpv.push_back({ 12,12,20,0,0 }); //inside the extent, but not covered by either raster. expected: filtered
			return lpv;
		};

		LidarPointVector lpv = freshPoints();
		
		Extent e{ 0,15,0,15,epsg2927 };

		auto demApplier = algo.getApplier(e, e.crs());
		Raster<coord_t> dem = *demApplier->getDem();

		demApplier->normalizePointVector(lpv);

		ASSERT_EQ(lpv.size(), 2);
		EXPECT_NEAR(lpv[0].z, 10, 0.1);
		EXPECT_NEAR(lpv[1].z, 1, 0.1);

		EXPECT_EQ(dem.xres(), 1);
		EXPECT_EQ(dem.yres(), 1);
		EXPECT_LE(dem.xmin(), e.xmin());
		EXPECT_GE(dem.xmax(), e.xmax());
		EXPECT_LE(dem.ymin(), e.ymin());
		EXPECT_GE(dem.ymax(), e.ymax());
		
		auto v = dem.extract(2, 2,ExtractMethod::near);
		EXPECT_TRUE(v.has_value());
		EXPECT_EQ(v.value(), 1);
		v = dem.extract(9, 9, ExtractMethod::near);
		EXPECT_TRUE(v.has_value());
		EXPECT_EQ(v.value(), 10);
		v = dem.extract(10.5, 10.5, ExtractMethod::near);
		EXPECT_FALSE(v.has_value());
	}

	TEST(DemAlgoTest, AlreadyNormalizedTest) {
		LidarPointVector lpv;
		lpv.push_back(LasPoint{ 1,1,-1,0,0 });
		lpv.push_back(LasPoint{ 2,2,101,0,0 });
		lpv.push_back(LasPoint{ 3,3,50,0,0 });

		AlreadyNormalized algo;
		algo.setMinMax(0, 100);

		auto applier = algo.getApplier(LasReader(Extent(0, 5, 0, 5)), CoordRef());
		Raster<coord_t> dem = *applier->getDem();
		applier->normalizePointVector(lpv);

		EXPECT_TRUE(dem.xmin() <= 0);
		EXPECT_TRUE(dem.xmax() >= 5);
		EXPECT_TRUE(dem.ymin() <= 0);
		EXPECT_TRUE(dem.ymax() >= 5);
		EXPECT_EQ(dem.ncell(), 1);
		EXPECT_TRUE(dem[0].has_value());
		EXPECT_EQ(dem[0].value(), 0);

		EXPECT_EQ(lpv.size(), 1);
		EXPECT_EQ(lpv[0].z, 50);
	}
}