#pragma once
#ifndef LP_TAOHANDLER_H
#define LP_TAOHANDLER_H

#include"ProductHandler.hpp"

namespace lapis {
	class TaoHandler : public ProductHandler {
	public:
		using ParamGetter = TaoParameterGetter;
		TaoHandler(ParamGetter* p);
		HANDLER_REGISTER_DECLARATION;

		void prepareForRun() override;
		void handlePoints(const std::span<LasPoint>& points, const Extent& e, size_t index) override;
		void finishLasFile(const Extent& e, size_t index) override;
		void handleDem(const Raster<coord_t>& dem, size_t index) override;
		void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) override;
		void cleanup() override;
		void reset() override;
		bool doThisProduct() override;
		std::string name() override;

		void describeInPdf(MetadataPdf& pdf) override;

		std::filesystem::path taoDir() const;
		std::filesystem::path taoTempDir() const;

	protected:

		ParamGetter* _getter;
        void _cleanupThreadFunc(cell_t tile) const;
        std::filesystem::path getHighPointFilename(cell_t tile) const;
        std::filesystem::path getSegmentRasterFilename(cell_t tile, TaoSegmentAlgorithm* segmenter, bool temp) const;
        std::filesystem::path getTaoHeightRasterFilename(cell_t tile, TaoSegmentAlgorithm* segmenter, bool temp) const;
        std::filesystem::path getSegmentPolygonFilename(cell_t tile, TaoSegmentAlgorithm* segmenter) const;

		std::string _highPointBasename = "TAOs";
		std::string _segmentsBasename = "Segments";
		std::string _taoHeightBasename = "TaoHeight";

		std::string _highPointFolderName = "Points";

		std::string _segmentRasterFolderName = "SegmentRasters";
		std::string _segmentPolygonFolderName = "SegmentPolygons";
		std::string _taoHeightFolderName = "TaoHeightRasters";


		/*struct TaoIdMap {
		using IDToCoord = std::unordered_map<taoid_t, cell_t>;
		std::unordered_map<cell_t, IDToCoord> tileToLocalNames;
		std::unordered_map<cell_t, taoid_t> cellToFinalName;
		};

		TaoIdMap idMap;

		struct TaoInfo {
		coord_t x, y;
		csm_t height;
		coord_t area;
		};

		void _writeHighPointsAsArray(const std::vector<cell_t>& highPoints, const Raster<csm_t>& bufferedCsm, const Raster<taoid_t>& bufferedSegments,
		const Extent& unbufferedExtent, cell_t tile) const;
		std::vector<TaoInfo> _readHighPointsFromArray(cell_t tile) const;

		void _updateMap(const Raster<taoid_t>& segments, const std::vector<cell_t>& highPoints, const Extent& unbufferedExtent, cell_t tileidx);
		Raster<taoid_t> _fixTaoIdsThread(cell_t tile) const;
		void _writeIdLayers(cell_t tile) const;*/
	};
}

#endif