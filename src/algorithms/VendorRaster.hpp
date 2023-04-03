#pragma once
#ifndef LP_VENDORRASTER_H
#define LP_VENDORRASTER_H

#include"DemAlgorithm.hpp"
#include"..\utils\MetadataPdf.hpp"

namespace lapis {

	template<class FILEGETTER>
	class VendorRasterApplier : public DemAlgoApplier {
	public:
		VendorRasterApplier(FILEGETTER* getter, LasReader&& l, const CoordRef& outCrs, coord_t minHt, coord_t maxHt);

		std::shared_ptr<Raster<coord_t>> getDem() override;

		//the span returned by this function will be valid until the next time getPoints is called
		std::span<LasPoint> getPoints(size_t n) override;
		size_t pointsRemaining() override;

		bool normalizePoint(LasPoint& p) override;

	private:
		FILEGETTER* _getter;
		std::shared_ptr<Raster<coord_t>> _dem;
		LidarPointVector _points;

		void _makeDem(const Extent& e);
	};

	//this class contains the implementation of the simplest dem algorithm: letting someone else do the work for you.
	//specifically, it takes in raster files from some other program as rasters, usually provided by the lidar vendor, and normalizes using those.
	template<class FILEGETTER>
	class VendorRaster : public DemAlgorithm {
	public:
		VendorRaster(FILEGETTER* getter);

		std::unique_ptr<DemAlgoApplier> getApplier(LasReader&& l, const CoordRef& outCrs) override;

		void describeInPdf(MetadataPdf& pdf) override;

	private:
		FILEGETTER* _getter;
	};

	template<class FILEGETTER>
	inline VendorRaster<FILEGETTER>::VendorRaster(FILEGETTER* getter) : _getter(getter)
	{
	}

	template<class FILEGETTER>
	inline std::unique_ptr<DemAlgoApplier> VendorRaster<FILEGETTER>::getApplier(LasReader&& l, const CoordRef& outCrs)
	{
		return std::make_unique<VendorRasterApplier<FILEGETTER>>(_getter, std::move(l), outCrs, _minHt, _maxHt);
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
	template<class FILEGETTER>
	inline VendorRasterApplier<FILEGETTER>::VendorRasterApplier(FILEGETTER* getter, LasReader&& l, const CoordRef& outCrs, coord_t minHt, coord_t maxHt)
		: DemAlgoApplier(std::move(l), outCrs, minHt, maxHt), _getter(getter)
	{
		if (!_getter) {
			throw std::invalid_argument("");
		}

		_makeDem(_las);
	}
	template<class FILEGETTER>
	inline std::shared_ptr<Raster<coord_t>> VendorRasterApplier<FILEGETTER>::getDem()
	{
		return _dem;
	}
	template<class FILEGETTER>
	inline std::span<LasPoint> VendorRasterApplier<FILEGETTER>::getPoints(size_t n)
	{
		_points = _las.getPoints(n);
		_points.transform(_crs);

		normalizePointVector(_points);

		return std::ranges::views::counted(_points.begin(), _points.size());
	}
	template<class FILEGETTER>
	inline size_t VendorRasterApplier<FILEGETTER>::pointsRemaining()
	{
		return _las.nPointsRemaining();
	}
	template<class FILEGETTER>
	inline bool VendorRasterApplier<FILEGETTER>::normalizePoint(LasPoint& p)
	{
		auto v = _dem->extract(p.x, p.y, ExtractMethod::bilinear);
		if (!v.has_value()) {
			return false;
		}
		p.z = p.z - v.value();
		return true;
	}
	template<class FILEGETTER>
	inline void VendorRasterApplier<FILEGETTER>::_makeDem(const Extent& e)
	{
		Extent projE = QuadExtent(_las, _crs).outerExtent();

		std::vector<Raster<coord_t>> overlappingDems;

		Alignment finestAlign = *(_getter->demAligns().begin());
		Extent buffer = bufferExtent(projE, std::max(finestAlign.xres(), finestAlign.yres()));
		finestAlign = finestAlign.transformAlignment(projE.crs());

		int index = -1;
		for (const Alignment& thisAlign : _getter->demAligns()) {
			++index;
			Alignment projAlign = thisAlign.transformAlignment(projE.crs());
			if (!projAlign.overlaps(buffer)) {
				continue;
			}

			std::optional<Raster<coord_t>> dem = _getter->getDem(index, buffer);
			if (!dem.has_value()) {
				continue;
			}
			if (!dem.value().crs().isConsistentHoriz(projE.crs())) {
				dem = dem.value().transformRaster(projE.crs(), ExtractMethod::bilinear);
			}
			overlappingDems.emplace_back(std::move(dem.value()));
		}

		Alignment outAlign = extendAlignment(finestAlign, projE, SnapType::out);
		outAlign = cropAlignment(outAlign, projE, SnapType::out);

		Alignment bufferedAlign = extendAlignment(outAlign, buffer, SnapType::out);

		_dem = std::make_shared<Raster<coord_t>>(bufferedAlign);

		for (Raster<coord_t>& dem : overlappingDems) {
			if (dem.crs().getZUnits() != _dem->crs().getZUnits()) {
				coord_t convFactor = convertUnits(1, dem.crs().getZUnits(), _dem->crs().getZUnits());
				for (cell_t cell = 0; cell < dem.ncell(); ++cell) {
					dem[cell].value() *= convFactor;
				}
			}
			Raster<coord_t> resampled;
			if (!_dem->isSameAlignment(dem)) {
				resampled = dem.resample(*_dem, ExtractMethod::bilinear);
			}
			const Raster<coord_t>* useRaster = _dem->isSameAlignment(dem) ? &dem : &resampled;

			_dem->overlay(*useRaster, [](coord_t a, coord_t b) {return a; });
		}
	}
}

#endif