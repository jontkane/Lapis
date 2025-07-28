#pragma once
#ifndef LP_HIGHPOINTS_H
#define LP_HIGHPOINTS_H

#include"TaoIdAlgorithm.hpp"

namespace lapis {

	class HighPoints : public TaoIdAlgorithm {

	public:
		HighPoints(coord_t minHtCsmZUnits, coord_t minDistCsmXYUnits);

		std::vector<IDedTao> identifyTaos(const Raster<csm_t>& csm, UniqueIdGenerator& idGenerator) override;

		void describeInPdf(MetadataPdf& pdf, TaoParameterGetter* getter) override;

		//for testing
		coord_t minHt() const;
		coord_t minDist() const;

	private:

		std::vector<IDedTao> _taoCandidates(const Raster<csm_t>& csm, UniqueIdGenerator& idGenerator) const;

		coord_t _minHt;
		coord_t _minDist;
	};
}

#endif