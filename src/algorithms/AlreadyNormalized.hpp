#pragma once
#ifndef LP_ALREADYNORMALIZED_H
#define LP_ALREADYNORMALIZED_H

#include"DemAlgorithm.hpp"

namespace lapis {

	class AlreadyNormalizedApplier : public DemAlgoApplier {
	public:

		AlreadyNormalizedApplier(LasReader&& l, const CoordRef& outCrs, coord_t minHt, coord_t maxHt);
		AlreadyNormalizedApplier(const Extent& e, const CoordRef& outCrs, coord_t minHt, coord_t maxHt);

		std::shared_ptr<Raster<coord_t>> getDem() override;
		std::span<LasPoint> getPoints(size_t n) override;
		size_t pointsRemaining() override;

		bool normalizePoint(LasPoint& p) override;


	private:
		LidarPointVector _points;
	};

	//The most basic possible algorithm: none. The input data is already normalized to the ground
	class AlreadyNormalized : public DemAlgorithm {
	public:

		std::unique_ptr<DemAlgoApplier> getApplier(LasReader&& l, const CoordRef& outCrs) override;

		void describeInPdf(MetadataPdf& pdf) override;
	};

	inline bool lapis::AlreadyNormalizedApplier::normalizePoint(LasPoint& p)
	{
		return true;
	}
}

#endif