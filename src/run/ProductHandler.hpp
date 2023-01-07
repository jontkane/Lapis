#pragma once
#ifndef LP_PRODUCT_HANDLER_H
#define LP_PRODUCT_HANDLER_H

#include"run_pch.hpp"
#include"..\parameters\ParameterGetter.hpp"

namespace lapis {

	class PointMetricCalculator;
	class MetricFunc;

	enum class OutputUnitLabel {
		Default, Radian, Percent, Unitless
	};

	class ProductHandler {
	public:
		using ParamGetter = SharedParameterGetter;
		ProductHandler(ParamGetter* p);
		virtual ~ProductHandler() = default;

		virtual void prepareForRun() = 0;
		//this function can assume that all points pass all filters, are normalized to the ground
		//are in the same projection (including Z units) as the extent, and are contained in the extent
		virtual void handlePoints(const LidarPointVector& points, const Extent& e, size_t index) = 0;
		virtual void handleDem(const Raster<coord_t>& dem, size_t index) = 0;
		virtual void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) = 0;
		virtual void cleanup() = 0;
		virtual void reset() = 0;

		std::filesystem::path parentDir() const;
		std::filesystem::path tempDir() const;

		//generates an appropriate filename for a "normal" metric
		std::filesystem::path getFullFilename(const std::filesystem::path& dir, const std::string& baseName,
			OutputUnitLabel u, const std::string& extension = "tif") const;
		//generates an appropriate filename for a temporary file which needs to be indexed for later recovery
		std::filesystem::path getFullTempFilename(const std::filesystem::path& dir, const std::string& baseName,
			OutputUnitLabel u, size_t index, const std::string& extension = "tif") const;
		//generates an appropriate filename for a file which needs to be indexed by the tile it represents
		std::filesystem::path getFullTileFilename(const std::filesystem::path& dir, const std::string& baseName,
			OutputUnitLabel u, cell_t tile, const std::string& extension = "tif") const;

	protected:
		ParamGetter* _sharedGetter;
		void deleteTempDirIfEmpty() const;

		template<class T>
		void writeRasterLogErrors(const std::filesystem::path& filename, Raster<T>& r) const;

		template<class T>
		Raster<T> getEmptyRasterFromTile(cell_t tile, const Alignment& a, coord_t minBufferMeters) const;
	};

	template<class T>
	inline void ProductHandler::writeRasterLogErrors(const std::filesystem::path& filename, Raster<T>& r) const
	{
		namespace fs = std::filesystem;
		LapisLogger& log = LapisLogger::getLogger();

		fs::create_directories(filename.parent_path());

		try {
			r.writeRaster(filename.string());
		}
		catch (InvalidRasterFileException e) {
			LapisLogger::getLogger().logMessage("Error writing " + filename.string());
		}
	}
	template<class T>
	inline Raster<T> ProductHandler::getEmptyRasterFromTile(cell_t tile, const Alignment& a, coord_t minBufferMeters) const
	{
		std::shared_ptr<Alignment> layout = _sharedGetter->layout();
		std::shared_ptr<Alignment> metricAlign = _sharedGetter->metricAlign();
		Extent thistile = layout->extentFromCell(tile);

		//The buffering ensures CSM metrics don't suffer from edge effects
		coord_t bufferDist = convertUnits(minBufferMeters, LinearUnitDefs::meter, layout->crs().getXYUnits());
		if (bufferDist > 0) {
			bufferDist = std::max(std::max(metricAlign->xres(), metricAlign->yres()), bufferDist);
		}
		Extent bufferExt = Extent(thistile.xmin() - bufferDist, thistile.xmax() + bufferDist, thistile.ymin() - bufferDist, thistile.ymax() + bufferDist);
		return Raster<T>(cropAlignment(a, bufferExt, SnapType::ll));
	}
}

#endif