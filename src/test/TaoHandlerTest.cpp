#include"test_pch.hpp"
#include"ParameterSpoofer.hpp"
#include"..\run\TaoHandler.hpp"
#include"..\algorithms\AllTaoIdAlgorithms.hpp"
#include"..\algorithms\AllTaoSegmentAlgorithms.hpp"

namespace lapis {

	class  TaoHandlerProtectedAccess : public TaoHandler {
	public:

		TaoHandlerProtectedAccess(ParamGetter* getter) : TaoHandler(getter) {}

		void writeArray(const std::vector<cell_t>& highPoints, const Raster<csm_t>& csm, const Raster<taoid_t>& segments,
			const Extent& unbufferedExtent, cell_t tile) {
			_writeHighPointsAsArray(highPoints, csm, segments, unbufferedExtent, tile);
		}
		std::vector<TaoInfo> readArray(cell_t tile) {
			return _readHighPointsFromArray(tile);
		}
		
		using TaoInfoProtected = TaoInfo;
	};

	void setReasonableTaoDefaults(TaoParameterSpoofer& spoof) {
		setReasonableSharedDefaults(spoof);
		spoof.setTaoIdAlgorithm(new HighPoints(2, 0));
		spoof.setTaoSegAlgorithm(new WatershedSegment(2, 100, 0.01));
	}

	TEST(TaoHandlerTest, xyzarraytest) {

		TaoParameterSpoofer spoof;
		setReasonableTaoDefaults(spoof);

		TaoHandlerProtectedAccess th(&spoof);
		th.prepareForRun();

		std::filesystem::remove_all(spoof.outFolder());

		Raster<csm_t> r{ Alignment(0,0,6,6,0.5,0.5) };
		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			r[cell].value() = (csm_t)cell;
			r[cell].has_value() = true;
		}

		Extent unbuffered = Extent(r.xmin(),
			(r.xmin() + r.xmax()) / 2,
			r.ymin(),
			(r.ymin() + r.ymax()) / 2);
		std::vector<cell_t> highPoints;
		std::vector<TaoHandlerProtectedAccess::TaoInfoProtected> expected;
		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			highPoints.push_back(cell);
			if (unbuffered.contains(r.xFromCell(cell), r.yFromCell(cell))) {
				expected.push_back({ r.xFromCell(cell), r.yFromCell(cell), r[cell].value(), 0.25*r.ncell()});
			}
		}

		Raster<taoid_t> fakeSegments((Alignment)r);
		for (cell_t cell = 0; cell < fakeSegments.ncell(); ++cell) {
			fakeSegments[cell].value() = 1;
			fakeSegments[cell].has_value() = true;
		}

		th.writeArray(highPoints, r, fakeSegments, unbuffered, 0);
		auto actual = th.readArray(0);

		ASSERT_EQ(expected.size(), actual.size());

		for (size_t i = 0; i < expected.size(); ++i) {
			EXPECT_EQ(expected[i].x, actual[i].x);
			EXPECT_EQ(expected[i].y, actual[i].y);
			EXPECT_EQ(expected[i].height, actual[i].height);
			EXPECT_EQ(expected[i].area, actual[i].area);
		}

		std::filesystem::remove_all(spoof.outFolder());
	}

	TEST(TaoHandlerTest, handletiletest) {
		TaoParameterSpoofer spoof;
		setReasonableTaoDefaults(spoof);

		TaoHandlerProtectedAccess th(&spoof);
		th.prepareForRun();

		std::filesystem::remove_all(spoof.outFolder());

		Raster<csm_t> fullCsm{ Alignment(0,0,6,6,0.5,0.5) };
		for (cell_t cell = 0; cell < fullCsm.ncell(); ++cell) {
			if (cell % 2 == 0) {
				fullCsm[cell].value() = (csm_t)cell;
				fullCsm[cell].has_value() = cell;
			}
		}

		cell_t testTile = 2;

		th.handleCsmTile(fullCsm, testTile);

		//the buffered csm here should just be all the tiles stitched together, so the outputs should be fairly easy to predict
		Extent tileExtent = spoof.layout()->extentFromCell(testTile);

		std::vector<cell_t> expectedHighPoints = spoof.taoIdAlgorithm()->identifyTaos(fullCsm);
		auto actualHighPoints = th.readArray(testTile);
		EXPECT_GT(actualHighPoints.size(), 0);
		for (size_t i = 0; i < expectedHighPoints.size(); ++i) {

			int foundCount = 0;
			int wantToFind = 0;

			if (tileExtent.contains(fullCsm.xFromCell(expectedHighPoints[i]), fullCsm.yFromCell(expectedHighPoints[i]))) {
				wantToFind = 1;
			}

			for (size_t j = 0; j < actualHighPoints.size(); ++j) {
				if (expectedHighPoints[i] == fullCsm.cellFromXY(actualHighPoints[j].x, actualHighPoints[j].y)) {
					foundCount++;
					EXPECT_NEAR(fullCsm[expectedHighPoints[i]].value(), actualHighPoints[j].height, 0.0001);
				}
			}
			EXPECT_EQ(foundCount, wantToFind);
		}

		GenerateIdByTile idGen{ spoof.layout()->ncell(),testTile };
		Raster<taoid_t> fullSegments = spoof.taoSegAlgorithm()->segment(fullCsm, expectedHighPoints, idGen);

		Raster<taoid_t> expectedSegments = cropRaster(fullSegments, tileExtent, SnapType::out);
		std::filesystem::path segmentsFile = th.getFullTileFilename(th.taoTempDir(), "Segments", OutputUnitLabel::Unitless, testTile);
		Raster<taoid_t> actualSegments = Raster<taoid_t>(segmentsFile.string());
		ASSERT_TRUE(expectedSegments.isSameAlignment(actualSegments));

		for (cell_t cell = 0; cell < expectedSegments.ncell(); ++cell) {
			EXPECT_TRUE(expectedSegments[cell].has_value() == actualSegments[cell].has_value());
			if (expectedSegments[cell].has_value()) {
				EXPECT_EQ(expectedSegments[cell].value(), actualSegments[cell].value());
			}
		}

		std::filesystem::path maxHeightFile = th.getFullTileFilename(th.taoDir(), "MaxHeight", OutputUnitLabel::Default, testTile);

		Raster<csm_t> actualMaxHeight = Raster<csm_t>(maxHeightFile.string());
		ASSERT_TRUE(actualMaxHeight.isSameAlignment(actualSegments));

		std::unordered_map<taoid_t, csm_t> heightBySegment;

		for (cell_t cell = 0; cell < actualMaxHeight.ncell(); ++cell) {
			EXPECT_TRUE(actualSegments[cell].has_value() == actualMaxHeight[cell].has_value());
			if (actualMaxHeight[cell].has_value()) {
				coord_t x = actualMaxHeight.xFromCell(cell);
				coord_t y = actualMaxHeight.yFromCell(cell);
				EXPECT_GE(actualMaxHeight[cell].value(), fullCsm.atXY(x, y).value());
				if (!heightBySegment.contains(actualSegments[cell].value())) {
					heightBySegment[actualSegments[cell].value()] = actualMaxHeight[cell].value();
				}
				else {
					EXPECT_EQ(heightBySegment[actualSegments[cell].value()], actualMaxHeight[cell].value());
				}
			}
		}

		std::filesystem::remove_all(spoof.outFolder());
	}

	TEST(TaoHandlerTest, cleanuptest) {
		//the strategy here is to do a basically full run, read in the high points and segments, and test that:
		//every high point is in the correct tile extent
		//every high point has a distinct ID
		//every pixel of the segments rasters has a value which belongs to a high point that made it all the way to the end

		TaoParameterSpoofer spoof;
		setReasonableTaoDefaults(spoof);

		TaoHandlerProtectedAccess th(&spoof);
		th.prepareForRun();

		std::filesystem::remove_all(spoof.outFolder());

		Raster<csm_t> fullCsm{ Alignment(0,0,6,6,0.5,0.5) };
		for (cell_t cell = 0; cell < fullCsm.ncell(); ++cell) {
			if (cell % 2 == 0) {
				fullCsm[cell].value() = (csm_t)cell;
				fullCsm[cell].has_value() = cell;
			}
		}

		for (cell_t tile = 0; tile < spoof.layout()->ncell(); ++tile) {
			th.handleCsmTile(fullCsm, tile);
		}

		th.cleanup();

		std::unordered_set<taoid_t> finalIDs;

		for (cell_t tile = 0; tile < spoof.layout()->ncell(); ++tile) {
			std::filesystem::path filename = th.getFullTileFilename(th.taoDir(), "TAOs", OutputUnitLabel::Unitless, tile, "shp");
			EXPECT_TRUE(std::filesystem::exists(filename));

			GDALDatasetWrapper highPoints = vectorGDALWrapper(filename.string());
			ASSERT_FALSE(highPoints.isNull());
			OGRLayer* layer = highPoints->GetLayer(0);
			OGRFeature* feature;
			while ((feature = layer->GetNextFeature()) != nullptr) {
				taoid_t id = feature->GetFieldAsInteger("ID");
				OGRPoint* geom = feature->GetGeometryRef()->toPoint();
				EXPECT_TRUE(spoof.layout()->extentFromCell(tile).contains(geom->getX(), geom->getY()));
				EXPECT_FALSE(finalIDs.contains(id));
				finalIDs.insert(id);
			}
			if (feature) {
				OGRFeature::DestroyFeature(feature);
			}
		}

		for (cell_t tile = 0; tile < spoof.layout()->ncell(); ++tile) {
			std::filesystem::path filename = th.getFullTileFilename(th.taoDir(), "Segments", OutputUnitLabel::Unitless, tile);
			ASSERT_TRUE(std::filesystem::exists(filename));

			Raster<taoid_t> segments{ filename.string() };
			for (cell_t cell = 0; cell < segments.ncell(); ++cell) {
				if (segments[cell].has_value()) {
					taoid_t v = segments[cell].value();
					EXPECT_TRUE(finalIDs.contains(v));
				}
			}
		}

		std::filesystem::remove_all(spoof.outFolder());
	}
}