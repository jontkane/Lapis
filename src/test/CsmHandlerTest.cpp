#include"test_pch.hpp"
#include"..\run\CsmHandler.hpp"
#include"..\algorithms\AllCsmAlgorithms.hpp"
#include"..\algorithms\AllCsmPostProcessors.hpp"
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

		spoof.setCsmAlign(Alignment(Extent(0, 3, 0, 3),0, 0, 0.5, 0.5));
		spoof.setDoCsm(true);
		spoof.setDoCsmMetrics(true);

		spoof.setCsmAlgo(new MaxPoint(0));
		spoof.setCsmPostProcessor(new FillCsm{ 6,5 });
	}

	TEST(CsmHandlerTest, constructortest) {
		CsmParameterSpoofer spoof;
		setReasonableCsmDefaults(spoof);

		CsmHandlerProtectedAccess ch(&spoof);
		ch.prepareForRun();

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
		ch.prepareForRun();

		namespace fs = std::filesystem;
		fs::remove_all(spoof.outFolder());
		fs::create_directory(spoof.outFolder());

		LidarPointVector points;
		Alignment a{ *spoof.csmAlign() };

		Extent lasExtent = *spoof.metricAlign();

		auto addPoint = [&](cell_t cell, csm_t value) {
			rowcol_t row = a.rowFromCell(cell);
			rowcol_t col = a.colFromCell(cell);
			if (!lasExtent.contains(a.xFromCol(col), a.yFromRow(row))) {
				return;
			}

			points.push_back(LasPoint{ a.xFromCell(cell),a.yFromCell(cell),value,0,0 });
		};
		for (cell_t cell = 0; cell < a.ncell(); ++cell) {
			addPoint(cell, (csm_t)cell);
		}

		auto csmMaker = spoof.csmAlgorithm()->getCsmMaker(a);
		csmMaker->addPoints(points);
		Raster<csm_t> expected = *csmMaker->currentCsm();

		ch.handlePoints(points, lasExtent, 0);
		ch.finishLasFile(lasExtent,0);

		fs::path expectedPath = ch.getFullTempFilename(ch.csmTempDir(), "CanopySurfaceModel", OutputUnitLabel::Default, 0);

		ASSERT_TRUE(fs::exists(expectedPath));
		Raster<csm_t> output{ expectedPath.string() };
		ASSERT_TRUE(output.xmin() <= expected.xmin());
		ASSERT_TRUE(output.xmax() >= expected.xmax());
		ASSERT_TRUE(output.ymin() <= expected.ymin());
		ASSERT_TRUE(output.ymax() >= expected.ymax());
		
		for (cell_t cell = 0; cell < expected.ncell(); ++cell) {
			coord_t x = expected.xFromCell(cell);
			coord_t y = expected.yFromCell(cell);
			auto vAct = output.atXY(x, y);
			auto vExp = expected[cell];
			EXPECT_EQ(vAct.has_value(), vExp.has_value());
			if (vExp.value()) {
				EXPECT_NEAR(vAct.value(), vExp.value(),0.0001);
			}
		}

		fs::remove_all(spoof.outFolder());
	}

	TEST(CsmHandlerTest, handletiletest) {
		CsmParameterSpoofer spoof;
		setReasonableCsmDefaults(spoof);
		spoof.setMetricAlign(Alignment(Extent(0, 4, 0, 4), 2, 2));
		spoof.setCsmPostProcessor(new DoNothingCsm());

		CsmHandlerProtectedAccess ch(&spoof);
		ch.prepareForRun();

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
		leftHalf.writeRaster(ch.getFullTempFilename(ch.csmTempDir(),"CanopySurfaceModel",OutputUnitLabel::Default,0).string());
		rightHalf.writeRaster(ch.getFullTempFilename(ch.csmTempDir(), "CanopySurfaceModel", OutputUnitLabel::Default, 1).string());


		Raster<csm_t> buffered = ch.getBufferedCsm(0);
		ASSERT_LE(buffered.xmin(), fullCsm.xmin() + LAPIS_EPSILON);
		ASSERT_GE(buffered.xmax(), fullCsm.xmax() - LAPIS_EPSILON);
		ASSERT_LE(buffered.ymin(), fullCsm.ymin() + LAPIS_EPSILON);
		ASSERT_GE(buffered.ymax(), fullCsm.ymax() - LAPIS_EPSILON);
		for (cell_t cell = 0; cell < fullCsm.ncell(); ++cell) {
			coord_t x = fullCsm.xFromCell(cell);
			coord_t y = fullCsm.yFromCell(cell);

			auto expV = fullCsm[cell];
			auto actV = buffered.atXY(x, y);

			EXPECT_EQ(actV.has_value(), expV.has_value());
			if (expV.has_value()) {
				EXPECT_EQ(actV.value(), expV.value());
			}
		}

		ch.handleCsmTile(buffered, 0);

		fs::path name = ch.getFullTileFilename(ch.csmDir(), "CanopySurfaceModel", OutputUnitLabel::Default, 0);
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