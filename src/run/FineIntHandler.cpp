#include"run_pch.hpp"
#include"FineIntHandler.hpp"
#include"..\parameters\RunParameters.hpp"
#include"LapisController.hpp"

namespace lapis {
	size_t FineIntHandler::handlerRegisteredIndex = LapisController::registerHandler(new FineIntHandler(&RunParameters::singleton()));
	void FineIntHandler::reset()
	{
		*this = FineIntHandler(_getter);
	}

	std::string FineIntHandler::name()
	{
		return "Fine Int";
	}

	FineIntHandler::FineIntHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;
	}
	void FineIntHandler::prepareForRun()
	{
		std::filesystem::remove_all(fineIntDir());
		std::filesystem::remove_all(fineIntTempDir());

		_fineIntBaseName = _getter->fineIntCanopyCutoff() > std::numeric_limits<coord_t>::lowest() ? "MeanCanopyIntensity" : "MeanIntensity";
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
	void FineIntHandler::describeInPdf(MetadataPdf& pdf)
	{
		if (!_getter->doFineInt()) {
			return;
		}
		pdf.newPage();
		pdf.writePageTitle("Intensity");
		pdf.writeTextBlockWithWrap("Intensity is a measure of how bright a lidar return is. It is unitless, and the scale varies from sensor to sensor, "
			"so the values should not be compared between acquisitions. A large number of things can result in one lidar return having lower intensity than another: "
			"the larger the distance between the sensor and ground, the lower the intensity; lidar pulses which strike slopes will have "
			"some of their light scatter and not return to the sensor, lowering the intensity; and the reflectance of the object struck by the lidar pulse "
			"will effect the amount of light returned to the sensor. Importantly, if this lidar uses near infrared light (as is usual), then "
			"chlorophyll is very reflective, and live vegetation will have higher intensity than dead vegetation or the ground (on average).");
		pdf.blankLine();
		
		std::stringstream productDesc;
		productDesc << "To reduce filesize, the intensity layers have been tiled. ";
		productDesc << "The tiles have names like: " << getFullTileFilename("", _fineIntBaseName, OutputUnitLabel::Unitless, 0) << ". ";
		productDesc << "The filename indicates the row and column of the tile. The location of each tile can be viewed in the file TileLayout.shp in the Layout directory. ";
		pdf.writeTextBlockWithWrap(productDesc.str());
		pdf.blankLine();
		productDesc.clear();
		productDesc.str("");

		productDesc << "The value of each cell is the mean intensity of the lidar returns that fall within that cell. ";
		if (_getter->fineIntCanopyCutoff() > -10000) {
			productDesc << "Returns less than " << pdf.numberWithUnits(_getter->fineIntCanopyCutoff(), _getter->unitSingular(), _getter->unitPlural());
			productDesc << " above the ground were ignored. This is usually to exclude the ground from the values, but if this is set high enough, it may ";
			productDesc << "exclude shrubs or some trees as well. ";
		}
		std::string cellsize = pdf.numberWithUnits(convertUnits(_getter->fineIntAlign()->xres(),
			_getter->fineIntAlign()->crs().getXYUnits(),
			_getter->outUnits()),
			_getter->unitSingular(),
			_getter->unitPlural());
		productDesc << "The cellsize of the intensity rasters is " << cellsize << ".";
		pdf.writeTextBlockWithWrap(productDesc.str());
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