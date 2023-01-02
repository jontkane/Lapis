#include"run_pch.hpp"
#include"FineIntHandler.hpp"

namespace lapis {
	FineIntHandler::FineIntHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;

		std::filesystem::remove_all(fineIntDir());
		std::filesystem::remove_all(fineIntTempDir());

		_fineIntBaseName = _getter->fineIntCanopyCutoff() > std::numeric_limits<coord_t>::lowest() ? "MeanCanopyIntensity_" : "MeanIntensity_";
	}
	void FineIntHandler::handlePoints(const LidarPointVector& points, const Extent& e, size_t index)
	{
		if (!_getter->doFineInt()) {
			return;
		}

		Raster<intensity_t> numerator{ cropAlignment(*_getter->fineIntAlign(),e, SnapType::out) };
		Raster<intensity_t> denominator{ (Alignment)numerator };

		coord_t cutoff = _getter->fineIntCanopyCutoff();
		for (const LasPoint& p : points) {
			if (p.z < cutoff) {
				continue;
			}
			cell_t cell = numerator.cellFromXYUnsafe(p.x, p.y);
			numerator[cell].value() += p.intensity;
			denominator[cell].value()++;
		}

		//there are many more points than cells, so rearranging work to be O(cells) instead of O(points) is generally worth it, even if it's a bit awkward
		for (cell_t cell = 0; cell < denominator.ncell(); ++cell) {
			if (denominator[cell].value() > 0) {
				denominator[cell].has_value() = true;
				numerator[cell].has_value() = true;
			}
		}

		writeRasterLogErrors(getFullTempFilename(fineIntTempDir(), _numeratorBasename, OutputUnitLabel::Unitless, index), numerator);
		writeRasterLogErrors(getFullTempFilename(fineIntTempDir(), _denominatorBasename, OutputUnitLabel::Unitless, index), denominator);
	}
	void FineIntHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{
	}
	void FineIntHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile)
	{

		if (!_getter->doFineInt()) {
			return;
		}

		Raster<intensity_t> numerator = getEmptyRasterFromTile<intensity_t>(tile, *_getter->fineIntAlign(), 0.);
		Raster<intensity_t> denominator{ (Alignment)numerator };

		Extent extentWithData;
		bool extentInit = false;

		const std::vector<Extent>& lasExtents = _getter->lasExtents();
		for (size_t i = 0; i < lasExtents.size(); ++i) {
			Extent thisext = lasExtents[i]; //intentional copy

			//Because the geotiff format doesn't store the entire WKT, you will sometimes end up in the situation where the WKT you set
			//and the WKT you get by writing and then reading a tif are not the same
			//This is mostly harmless but it does cause proj's is_equivalent functions to return false sometimes
			//In this case, we happen to know that the files we're going to be reading are the same CRS as thisext, regardless of slight WKT differences,
			//so we give it an empty CRS to force all isConsistent functions to return true
			thisext.defineCRS(CoordRef());

			Raster<intensity_t> thisNumerator, thisDenominator;

			if (!thisext.overlaps(numerator)) {
				continue;
			}
			thisext = cropExtent(thisext, numerator);

			try {
				thisNumerator = Raster<intensity_t>{ getFullTempFilename(fineIntTempDir(),_numeratorBasename,OutputUnitLabel::Unitless,i).string(),
					numerator, SnapType::near};
				thisDenominator = Raster<intensity_t>{ getFullTempFilename(fineIntTempDir(),_denominatorBasename,OutputUnitLabel::Unitless,i).string(),
					numerator, SnapType::near };
			}
			catch (InvalidRasterFileException e) {
				LapisLogger::getLogger().logMessage("Issue opening temporary intensity file " + std::to_string(i));
				continue;
			}

			numerator.overlay(thisNumerator, [](intensity_t a, intensity_t b) {return a + b; });
			denominator.overlay(thisDenominator, [](intensity_t a, intensity_t b) {return a + b; });

			if (!extentInit) {
				extentInit = true;
				extentWithData = numerator;
			}
			else {
				extentWithData = extendExtent(extentWithData, numerator);
			}
		}

		if (!denominator.hasAnyValue()) {
			return;
		}

		Raster<intensity_t> meanIntensity = numerator / denominator;
		meanIntensity = cropRaster(meanIntensity, extentWithData, SnapType::out);
		writeRasterLogErrors(getFullTileFilename(fineIntDir(), _fineIntBaseName, OutputUnitLabel::Unitless, tile), meanIntensity);
	}
	void FineIntHandler::cleanup() {
		std::filesystem::remove_all(fineIntTempDir());
		deleteTempDirIfEmpty();
	}
	std::filesystem::path FineIntHandler::fineIntDir() const
	{
		return parentDir() / "Intensity";
	}
	std::filesystem::path FineIntHandler::fineIntTempDir() const
	{
		return tempDir() / "Intensity";
	}
}