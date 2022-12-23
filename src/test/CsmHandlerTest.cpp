#include<gtest/gtest.h>
#include"..\ProductHandler.hpp"
#include"ParameterSpoofer.hpp"

namespace lapis {
	class CsmHandlerProtectedAccess : public CsmHandler {
	public:
		CsmHandlerProtectedAccess(ParamGetter* getter) : CsmHandler(getter) {}

		std::vector<CSMMetricRaster>& csmMetrics() {
			return _csmMetrics;
		}
	};

	void setReasonableCsmDefaults(CsmParameterSpoofer& spoof) {
		setReasonableSharedDefaults(spoof);

		coord_t footprintRadius = 0.71;
		spoof.setCsmAlign(Alignment(
			Extent(-footprintRadius, 3 + footprintRadius, -footprintRadius, 3 + footprintRadius),
			0, 0, 0.5, 0.5));
		spoof.setDoCsm(true);
		spoof.setDoCsmMetrics(true);
		spoof.setFootprintDiameter(footprintRadius*2); //just barely large enough to reach all eight directions if places at the center of a pixel
		spoof.setSmooth(1);
	}

	TEST(CsmHandlerTest, constructortest) {
		CsmParameterSpoofer spoof;
		setReasonableCsmDefaults(spoof);

		CsmHandlerProtectedAccess ch(&spoof);

		EXPECT_GT(ch.csmMetrics().size(), 0);
		for (size_t i = 0; i < ch.csmMetrics().size(); ++i) {
			EXPECT_EQ((Alignment)ch.csmMetrics()[i].raster, *spoof.metricAlign());
		}

		spoof.setDoCsmMetrics(false);
		ch = CsmHandlerProtectedAccess(&spoof);
		EXPECT_EQ(ch.csmMetrics().size(), 0);
	}

	TEST(CsmHandlerTest, handlepointstest) {
		CsmParameterSpoofer spoof;
		setReasonableCsmDefaults(spoof);

		CsmHandlerProtectedAccess ch(&spoof);

		namespace fs = std::filesystem;
		fs::remove_all(spoof.outFolder());
		fs::create_directory(spoof.outFolder());

		coord_t footprintRadius = spoof.footprintDiameter() / 2;
		LidarPointVector points;
		Raster<csm_t> expected{ *spoof.csmAlign() };

		Extent lasExtent = *spoof.metricAlign();

		auto addPoint = [&](cell_t cell, csm_t value) {
			rowcol_t row = expected.rowFromCell(cell);
			rowcol_t col = expected.colFromCell(cell);
			if (!lasExtent.contains(expected.xFromCol(col), expected.yFromRow(row))) {
				return;
			}

			//this loop is the expected behavior of the footprint spreading because of a carefully chosen footprint value
			for (rowcol_t rownudge : {row - 1, row, row + 1}) {
				if (rownudge < 0 || rownudge >= expected.nrow()) {
					continue;
				}
				for (rowcol_t colnudge : {col - 1, col, col + 1}) {
					if (colnudge < 0 || colnudge >= expected.ncol()) {
						continue;
					}
					auto v = expected.atRC(rownudge, colnudge);
					v.has_value() = true;
					v.value() = std::max(v.value(), value);
				}
			}

			points.push_back(LasPoint{ expected.xFromCell(cell),expected.yFromCell(cell),value,0,0 });
		};
		for (cell_t cell = 0; cell < expected.ncell(); ++cell) {
			addPoint(cell, (csm_t)cell);
		}
		expected = smoothAndFill(expected, spoof.smooth(), 6, {});

		ch.handlePoints(points, lasExtent, 0);

		ASSERT_TRUE(fs::exists(ch.csmTempDir() / "0.tif"));
		Raster<csm_t> output{ (ch.csmTempDir() / "0.tif").string() };
		ASSERT_TRUE(output.isSameAlignment(expected));
		
		for (cell_t cell = 0; cell < expected.ncell(); ++cell) {
			EXPECT_EQ(output[cell].has_value(), expected[cell].has_value());
			if (expected[cell].value()) {
				EXPECT_NEAR(output[cell].value(), expected[cell].value(),0.0001);
			}
		}

		fs::remove_all(spoof.outFolder());
	}

	TEST(CsmHandlerTest, handletiletest) {
		CsmParameterSpoofer spoof;
		setReasonableCsmDefaults(spoof);

		CsmHandlerProtectedAccess ch(&spoof);

		namespace fs = std::filesystem;
		fs::remove_all(spoof.outFolder());
		fs::create_directory(spoof.outFolder());

		Extent tileExtent = spoof.layout()->extentFromCell(0);
		Raster<csm_t> fullCsm{ *spoof.csmAlign() };
		for (cell_t cell = 0; cell < fullCsm.ncell(); ++cell) {
			if (cell % 2 == 1) {
				fullCsm[cell].has_value() = true;
				fullCsm[cell].value() = (csm_t)cell;
			}
		}
		Raster<csm_t> leftHalf = cropRaster(fullCsm,Extent(fullCsm.xmin(),(fullCsm.xmin()+fullCsm.xmax())/2,
			fullCsm.ymin(), fullCsm.ymax()), SnapType::ll);
		Raster<csm_t> rightHalf = cropRaster(fullCsm, Extent((fullCsm.xmin() + fullCsm.xmax()) / 2, fullCsm.xmax(),
			fullCsm.ymin(), fullCsm.ymax()), SnapType::ll);

		spoof.addLasExtent(leftHalf);
		spoof.addLasExtent(rightHalf);

		fs::create_directories(ch.csmTempDir());
		leftHalf.writeRaster((ch.csmTempDir() / "0.tif").string());
		rightHalf.writeRaster((ch.csmTempDir() / "1.tif").string());


		Raster<csm_t> buffered = ch.getBufferedCsm(0);
		ASSERT_TRUE(buffered.isSameAlignment(fullCsm));
		for (cell_t cell = 0; cell < buffered.ncell(); ++cell) {
			EXPECT_EQ(buffered[cell].has_value(), fullCsm[cell].has_value());
			if (fullCsm[cell].has_value()) {
				EXPECT_EQ(buffered[cell].value(), fullCsm[cell].value());
			}
		}

		ch.handleCsmTile(buffered, 0);

		fs::path name = ch.csmDir() / (spoof.name() + "_CanopySurfaceModel_" + nameFromLayout(*spoof.layout(), 0) + "_Meters.tif");
		ASSERT_TRUE(fs::exists(name));
		Raster<csm_t> actual{ name.string() };
		Raster<csm_t> expected = cropRaster(fullCsm, tileExtent, SnapType::out);

		ASSERT_TRUE(actual.isSameAlignment(expected));
		for (cell_t cell = 0; cell < actual.ncell(); ++cell) {
			EXPECT_EQ(actual[cell].has_value(), expected[cell].has_value());
			if (expected[cell].has_value()) {
				EXPECT_EQ(actual[cell].value(), expected[cell].value());
			}
		}

		fs::remove_all(spoof.outFolder());

		//the actual CSM summary functions are tested elsewhere. I just want to verify they're being called
		EXPECT_TRUE(ch.csmMetrics()[0].raster.hasAnyValue());

	}

}