#pragma once
#ifndef LP_VENDORRASTER_H
#define LP_VENDORRASTER_H

#include"DemAlgorithm.hpp"

namespace lapis {

	//this class contains the implementation of the simplest dem algorithm: letting someone else do the work for you.
	//specificlly, it takes in raster files from some other program as rasters, usually provided by the lidar vendor, and normalizes using those.

	template<class FILEGETTER>
	class VendorRaster : public DemAlgorithm {
	public:
		VendorRaster(FILEGETTER* getter);

		PointsAndDem normalizeToGround(const LidarPointVector& points, const Extent& e) override;

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
	inline DemAlgorithm::PointsAndDem VendorRaster<FILEGETTER>::normalizeToGround(const LidarPointVector& points, const Extent& e) {
		DemAlgorithm::PointsAndDem out;

		coord_t minRes = std::numeric_limits<coord_t>::max();
		coord_t xOriginOfFinest = 0;
		coord_t yOriginOfFinest = 0;
		std::vector<Raster<coord_t>> overlappingDems;
		for (size_t i = 0; i < _getter->demAligns().size(); ++i) {
			Alignment projAlign = _getter->demAligns()[i].transformAlignment(e.crs());
			if (!projAlign.overlaps(e)) {
				continue;
			}
			if (projAlign.xres() < minRes || projAlign.yres() < minRes) {
				minRes = std::min(projAlign.xres(), projAlign.yres());
				xOriginOfFinest = projAlign.xOrigin();
				yOriginOfFinest = projAlign.yOrigin();
			}

			Raster<coord_t> demOriginalProj = _getter->getDem(i);
			overlappingDems.push_back(demOriginalProj.transformRaster(e.crs(), ExtractMethod::bilinear));
		}

		std::sort(overlappingDems.begin(), overlappingDems.end(),
			[](const auto& a, const auto& b) {return a.xres() * a.yres() < b.xres() * b.yres(); });
		//at the end of all this, the list should now be sorted by cellsize. DEMs in the same CRS as the points should have had to undergo any loss of precision

		out.dem = Raster<coord_t>{ Alignment(e,xOriginOfFinest,yOriginOfFinest,minRes,minRes) };
		for (const Raster<coord_t>& dem : overlappingDems) {
			Raster<coord_t> resampled;
			if (!out.dem.isSameAlignment(dem)) {
				resampled = dem.resample(out.dem, ExtractMethod::bilinear);
			}
			const Raster<coord_t>* useRaster = out.dem.isSameAlignment(dem) ? &dem : &resampled;

			out.dem.overlay(*useRaster, [](coord_t a, coord_t b) {return a; });
		}

		for (const LasPoint& point : points) {
			auto v = out.dem.extract(point.x, point.y, ExtractMethod::bilinear);
			if (!v.has_value()) {
				continue;
			}

			coord_t z = point.z - v.value();
			if (z > _maxHt) {
				continue;
			}
			if (z < _minHt) {
				continue;
			}
			out.points.emplace_back(
				point.x,
				point.y,
				z,
				point.intensity,
				point.returnNumber);
		}

		return out;
	}

	class EmptyFileGetter {
	public:
		const std::vector<Alignment>& demAligns() { return _aligns; }
		Raster<coord_t> getDem(size_t n) { return Raster<coord_t>(); }

	private:
		std::vector<Alignment> _aligns;
	};
	template VendorRaster<EmptyFileGetter>;
}

#endif