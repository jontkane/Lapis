#pragma once
#ifndef LP_VENDORRASTER_H
#define LP_VENDORRASTER_H

#include"DemAlgorithm.hpp"
#include"..\utils\MetadataPdf.hpp"

namespace lapis {

	//this class contains the implementation of the simplest dem algorithm: letting someone else do the work for you.
	//specificlly, it takes in raster files from some other program as rasters, usually provided by the lidar vendor, and normalizes using those.

	template<class FILEGETTER>
	class VendorRaster : public DemAlgorithm {
	public:
		VendorRaster(FILEGETTER* getter);

		Raster<coord_t> normalizeToGround(LidarPointVector& points, const Extent& e) override;

		void describeInPdf(MetadataPdf& pdf) override;

	private:
		FILEGETTER* _getter;
	};

	template<class FILEGETTER>
	inline VendorRaster<FILEGETTER>::VendorRaster(FILEGETTER* getter)
		: _getter(getter)
	{
		if (!_getter) {
			throw std::invalid_argument("");
		}
	}

	template<class FILEGETTER>
	inline Raster<coord_t> VendorRaster<FILEGETTER>::normalizeToGround(LidarPointVector& points, const Extent& e) {

		std::vector<Raster<coord_t>> overlappingDems;

		Alignment finestAlign = *(_getter->demAligns().begin());
		Extent buffer = bufferExtent(e, std::max(finestAlign.xres(), finestAlign.yres()));
		finestAlign = finestAlign.transformAlignment(e.crs());

		int index = -1;
		for (const Alignment& thisAlign : _getter->demAligns()) {
			++index;
			Alignment projAlign = thisAlign.transformAlignment(e.crs());
			if (!projAlign.overlaps(buffer)) {
				continue;
			}

			std::optional<Raster<coord_t>> dem = _getter->getDem(index, buffer);
			if (!dem.has_value()) {
				continue;
			}
			if (!dem.value().crs().isConsistentHoriz(e.crs())) {
				dem = dem.value().transformRaster(e.crs(), ExtractMethod::bilinear);
			}
			overlappingDems.emplace_back(std::move(dem.value()));
		}

		Alignment outAlign = extendAlignment(finestAlign, e, SnapType::out);
		outAlign = cropAlignment(outAlign, e, SnapType::out);

		Alignment bufferedAlign = extendAlignment(outAlign, buffer, SnapType::out);

		Raster<coord_t> outDem = Raster<coord_t>{ bufferedAlign };

		for (Raster<coord_t>& dem : overlappingDems) {
			if (dem.crs().getZUnits() != outDem.crs().getZUnits()) {
				coord_t convFactor = convertUnits(1, dem.crs().getZUnits(), outDem.crs().getZUnits());
				for (cell_t cell = 0; cell < dem.ncell(); ++cell) {
					dem[cell].value() *= convFactor;
				}
			}
			Raster<coord_t> resampled;
			if (!outDem.isSameAlignment(dem)) {
				resampled = dem.resample(outDem, ExtractMethod::bilinear);
			}
			const Raster<coord_t>* useRaster = outDem.isSameAlignment(dem) ? &dem : &resampled;

			outDem.overlay(*useRaster, [](coord_t a, coord_t b) {return a; });
		}

		auto normalize = [&outDem](LasPoint& point)->bool {
			auto v = outDem.extract(point.x, point.y, ExtractMethod::bilinear);
			if (!v.has_value()) {
				return false;
			}
			point.z = point.z - v.value();
			return true;
		};

		_normalizeByFunction(points, normalize);

		return cropRaster(outDem,outAlign,SnapType::near);
	}

	template<class FILEGETTER>
	inline void VendorRaster<FILEGETTER>::describeInPdf(MetadataPdf& pdf)
	{
		pdf.newPage();
		pdf.writePageTitle("Ground Model Algorithm");

		pdf.writeTextBlockWithWrap("In this run, Lapis did not calculate its own ground model. "
			"Instead, the user supplied a rasterized ground model produced by previous software. "
			"The elevation of the ground beneath each point was estimated with a bilinear interpolation from those rasters.");
	}


	//this class exists to make VendorRaster easier to navigate in IDEs, because DemParameter is not in the same library
	//it shows the skeleton of the minimum VendorRaster needs to function
	class EmptyFileGetter {
	public:
		//this function doesn't have to return a vector; it just needs to return something iterable
		//the list should be sorted though, so that, when multiple dems overlap, the one that appears earlier is the correct one to use
		const std::vector<Alignment>& demAligns() { return _aligns; }

		//this should return the dem corresponding to the Nth element in the container demAligns returns
		std::optional<Raster<coord_t>> getDem(size_t n, const Extent& e) { return Raster<coord_t>(); }

	private:
		std::vector<Alignment> _aligns;
	};
	template VendorRaster<EmptyFileGetter>;
}

#endif