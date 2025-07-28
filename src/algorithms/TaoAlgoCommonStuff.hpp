#pragma once
#ifndef LP_TAOALGOCOMMONSTUFF_H
#define LP_TAOALGOCOMMONSTUFF_H

#include"algo_pch.hpp"

namespace lapis {
	struct IDedTao {
		cell_t location;
		taoid_t id;
	};


	class UniqueIdGenerator {
	public:
		virtual ~UniqueIdGenerator() = default;

		virtual taoid_t nextId() = 0;
	};

	class GenerateIdByTile : public UniqueIdGenerator {
	public:
		GenerateIdByTile(cell_t nTiles, cell_t thisTile)
			: _nTiles((taoid_t)nTiles), _previousId((taoid_t)(thisTile - nTiles + 1))
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

	struct SegmentResults {
		std::optional<Raster<taoid_t>> raster;
		std::optional<VectorDataset<MultiPolygon>> vector;
	};
}

#endif