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
	};
}

#endif