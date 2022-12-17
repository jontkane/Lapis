#include"app_pch.hpp"
#include"demalgos.hpp"
#include"LapisData.hpp"
#include"LapisLogger.hpp"

namespace lapis {
	PointsAndDem VendorRaster::normalizeToGround(const LidarPointVector& points, const Extent& e)
	{
		LapisData& data = LapisData::getDataObject();
		LapisLogger& log = LapisLogger::getLogger();

		if (data.demList().size() == 0) {
			log.logMessage("No DEM files specified");
			data.needAbort = true;
			return PointsAndDem();
		}

		PointsAndDem out;

		coord_t minRes = std::numeric_limits<coord_t>::max();
		coord_t xOriginOfFinest = 0;
		coord_t yOriginOfFinest = 0;
		std::vector<Raster<coord_t>> overlappingDems;
		for (const DemFileAlignment& demAlign : data.demList()) {
			Alignment projAlign = demAlign.align.transformAlignment(e.crs());
			if (!projAlign.overlaps(e)) {
				continue;
			}
			if (projAlign.xres() < minRes || projAlign.yres() < minRes) {
				minRes = std::min(projAlign.xres(), projAlign.yres());
				xOriginOfFinest = projAlign.xOrigin();
				yOriginOfFinest = projAlign.yOrigin();
			}

			Raster<coord_t> demOriginalProj = Raster<coord_t>(demAlign.filename, QuadExtent(e, demAlign.align.crs()).outerExtent(), SnapType::out);
			if (!data.demCrsOverride().isEmpty()) {
				demOriginalProj.defineCRS(data.demCrsOverride());
			}
			if (!data.demUnitOverride().isUnknown()) {
				demOriginalProj.setZUnits(data.demUnitOverride());
			}
			overlappingDems.push_back(demOriginalProj.transformRaster(e.crs(),ExtractMethod::bilinear));
		}

		std::sort(overlappingDems.begin(), overlappingDems.end(),
			[](const auto& a, const auto& b) {return a.xres() * a.yres() < b.xres() * b.yres(); });
		//at the end of all this, the list should now be sorted by cellsize. DEMs in the same CRS as the points should have had to undergo any loss of precision

		out.dem = Raster<coord_t>{ Alignment(e,xOriginOfFinest,yOriginOfFinest,minRes,minRes) };
		for (const Raster<coord_t>& dem : overlappingDems) {
			Raster<coord_t> resampled;
			if (!out.dem.consistentAlignment(dem)) {
				resampled = dem.resample(out.dem, ExtractMethod::bilinear);
			}
			const Raster<coord_t>* useRaster = out.dem.consistentAlignment(dem) ? &dem : &resampled;

			out.dem.overlay(*useRaster, [](coord_t a, coord_t b) {return a; });
		}

		for (const LasPoint& point : points) {
			auto v = out.dem.extract(point.x, point.y, ExtractMethod::bilinear);
			if (!v.has_value()) {
				continue;
			}

			coord_t z = point.z - v.value();
			if (z > data.maxHt()) {
				continue;
			}
			if (z < data.minHt()) {
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
}