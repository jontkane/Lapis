#pragma once
#ifndef LP_TAOHANDLER_H
#define LP_TAOHANDLER_H

#include"ProductHandler.hpp"

namespace lapis {
	class TaoHandler : public ProductHandler {
	public:
		using ParamGetter = TaoParameterGetter;
		TaoHandler(ParamGetter* p);

		void handlePoints(const LidarPointVector& points, const Extent& e, size_t index) override;
		void handleDem(const Raster<coord_t>& dem, size_t index) override;
		void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) override;
		void cleanup() override;
		void reset() override;
		static size_t handlerRegisteredIndex;

		std::filesystem::path taoDir() const;
		std::filesystem::path taoTempDir() const;

	protected:
		struct TaoIdMap {
			struct XY { coord_t x, y; };
			struct XYHasher {
				std::size_t operator() (const XY& coord) const {
					return (std::hash<coord_t>()(coord.x) >> 1) ^ (std::hash<coord_t>()(coord.y));
				}
			};
			struct XYEqual {
				bool operator() (const XY& lhs, const XY& rhs) const {
					return lhs.x == rhs.x && lhs.y == rhs.y;
				}
			};
			using IDToCoord = std::unordered_map<taoid_t, XY>;
			std::unordered_map<cell_t, IDToCoord> tileToLocalNames;
			std::unordered_map<XY, taoid_t, XYHasher, XYEqual> coordsToFinalName;
		};

		TaoIdMap idMap;

		void _writeHighPointsAsXYZArray(const std::vector<cell_t>& highPoints, const Raster<csm_t>& csm, const Extent& unbufferedExtent, cell_t tile) const;
		std::vector<CoordXYZ> _readHighPointsFromXYZArray(cell_t tile) const;

	private:
		ParamGetter* _getter;

		void _updateMap(const Raster<taoid_t>& segments, const std::vector<cell_t>& highPoints, const Extent& unbufferedExtent, cell_t tileidx);
		void _fixTaoIdsThread(cell_t tile) const;
		void _writeHighPointsAsShp(const Raster<taoid_t>& segments, cell_t tile) const;

		std::string _taoBasename = "TAOs";
		std::string _segmentsBasename = "Segments";
		std::string _maxHeightBasename = "MaxHeight";
	};
}

#endif