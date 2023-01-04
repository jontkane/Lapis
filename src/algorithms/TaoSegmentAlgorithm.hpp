#pragma once
#ifndef LP_TAOSEGMENTALGORITHM_H
#define LP_TAOSEGMENTALGORITHM_H

#include"algo_pch.hpp"

namespace lapis {

	class UniqueIdGenerator;

	class TaoSegmentAlgorithm {
	public:

		virtual ~TaoSegmentAlgorithm() = default;

		virtual Raster<taoid_t> segment(const Raster<csm_t>& csm, const std::vector<cell_t>& taos, UniqueIdGenerator& idGenerator) = 0;
	};

	class UniqueIdGenerator {
	public:
		virtual ~UniqueIdGenerator() = default;

		virtual taoid_t nextId() = 0;
	};

	class GenerateIdByTile : public UniqueIdGenerator {
	public:
		GenerateIdByTile(cell_t nTiles, cell_t thisTile)
			: _nTiles((taoid_t)nTiles), _previousId((taoid_t)(thisTile - nTiles))
		{
		}
		taoid_t nextId()
		{
			_previousId += _nTiles;
			return _previousId;
		}
	private:
		taoid_t _nTiles;
		taoid_t _previousId;
	};
}

#endif