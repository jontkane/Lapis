#pragma once
#ifndef LP_DEMALGORITHM_H
#define LP_DEMALGORITHM_H

#include"algo_pch.hpp"

namespace lapis {

	class MetadataPdf;

	class DemAlgoApplier {
	public:

		virtual ~DemAlgoApplier() = default;

		DemAlgoApplier(LasReader&& l, const CoordRef& outCrs, coord_t minHt, coord_t maxHt) : _las(std::move(l)), _crs(outCrs), _minHt(minHt), _maxHt(maxHt) {}
		DemAlgoApplier() = default;

		virtual std::shared_ptr<Raster<coord_t>> getDem() = 0;

		//the span returned by this function will be valid until the next time getPoints is called
		virtual std::span<LasPoint> getPoints(size_t n) = 0;
		virtual size_t pointsRemaining() = 0;

		//this function is split off mostly to aid with testing
		virtual void normalizePointVector(LidarPointVector& points);
		virtual bool normalizePoint(LasPoint& p) = 0;

	protected:

		LasReader _las;
		CoordRef _crs;
		coord_t _minHt = 0;
		coord_t _maxHt = 300;
	};
	
	//This class is intended to be a stand-between a LasReader and the code which actually uses the las points
	//Because different dem algorithms have different needs (e.g., some need to read all points at the start, some can stream them),
	//this class is responsible for handling that
	class DemAlgorithm {
	public:

		virtual ~DemAlgorithm() = default;

		virtual std::unique_ptr<DemAlgoApplier> getApplier(LasReader&& l, const CoordRef& outCrs) = 0;

		virtual void describeInPdf(MetadataPdf& pdf) = 0;

		void setMinMax(coord_t minHt, coord_t maxHt) {
			_minHt = minHt;
			_maxHt = maxHt;
		}

	protected:
		coord_t _minHt = 0;
		coord_t _maxHt = 300;
	};
}

#endif