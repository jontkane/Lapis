#pragma once
#ifndef LP_PRODUCT_HANDLER_H
#define LP_PRODUCT_HANDLER_H

#include"app_pch.hpp"
#include"LapisData.hpp"
#include"csmalgos.hpp"

namespace lapis {

	std::string nameFromLayout(cell_t cell);

	class ProductHandler {
	public:
		//this function can assume that all points pass all filters, are normalized to the ground
		//are in the same projection (including Z units) as the extent, and are contained in the extent
		virtual void handlePoints(const LidarPointVector& points, const Raster<coord_t>& dem, const Extent& e, size_t index) = 0;
		virtual void handleTile(cell_t tile) = 0;
		virtual void cleanup() = 0;

		static std::filesystem::path parentDir();
		static std::filesystem::path tempDir();

	protected:
		void deleteTempDirIfEmpty() const;
	};

	class PointMetricHandler : public ProductHandler {
	public:
		PointMetricHandler();

		void handlePoints(const LidarPointVector& points, const Raster<coord_t>& dem, const Extent& e, size_t index) override;
		void handleTile(cell_t tile) override;
		void cleanup() override;

		
		static std::filesystem::path pointMetricDir();
	};

	class CsmHandler : public ProductHandler {
	public:
		void handlePoints(const LidarPointVector& points, const Raster<coord_t>& dem, const Extent& e, size_t index) override;
		void handleTile(cell_t tile) override;
		void cleanup() override;

		static std::filesystem::path csmTempDir();
		static std::filesystem::path csmDir();
		static std::filesystem::path csmMetricDir();
	};

	//This class must be placed *after* CsmHandler in the order they're run
	class TaoHandler : public ProductHandler {
	public:
		void handlePoints(const LidarPointVector& points, const Raster<coord_t>& dem, const Extent& e, size_t index) override;
		void handleTile(cell_t tile) override;
		void cleanup() override;

		static std::filesystem::path taoDir();
		static std::filesystem::path taoTempDir();

	private:
		TaoIdMap idMap;
		void _updateMap(const Raster<taoid_t>& segments, const std::vector<cell_t>& highPoints, const Extent& unbufferedExtent, cell_t tileidx);
		void _fixTaoIdsThread(cell_t tile) const;
	};

	class FineIntHandler : public ProductHandler {
	public:
		void handlePoints(const LidarPointVector& points, const Raster<coord_t>& dem, const Extent& e, size_t index) override;
		void handleTile(cell_t tile) override;
		void cleanup() override;

		static std::filesystem::path fineIntDir();
		static std::filesystem::path fineIntTempDir();
	};

	class TopoHandler : public ProductHandler {
	public:
		void handlePoints(const LidarPointVector& points, const Raster<coord_t>& dem, const Extent& e, size_t index) override;
		void handleTile(cell_t tile) override;
		void cleanup() override;

		static std::filesystem::path topoDir();
	};
}

#endif