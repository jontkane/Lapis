#pragma once
#ifndef LP_RUNPARAMETERS_H
#define LP_RUNPARAMETERS_H

#include"param_pch.hpp"
#include"Parameter.hpp"
#include"ParameterGetter.hpp"
#include"..\utils\MetadataPdf.hpp"

namespace lapis {

	class DemAlgorithm;

	class LapisParameters : public ParameterManager {

	public:
		LapisParameters();
		LapisParameters(const LapisParameters&) = delete;
		LapisParameters(LapisParameters&&) = delete;

		bool prepareForRun() override;
		void cleanAfterRun() override;
		void reset() override;

		void importBoostAndUpdateUnits() override;
		void updateUnits() override;
		void setPrevUnits(const LinearUnit& u) override;
		const LinearUnit& prevUnits() override;

		const LinearUnit& outUnits() override;
		const std::string& unitSingular() override;
		const std::string& unitPlural() override;

		const std::vector<Extent>& lasExtents() override;
		LasReader getLas(size_t i) override;
		std::optional<LinearUnit> lasZUnits() override;

		std::unique_ptr<DemAlgoApplier> demAlgorithm(LasReader&& l) override;

		const CoordRef& userCrsSpecification() override;
		const CoordRef& outputCrs() override;

		const Extent& fullExtent() override;
		const std::shared_ptr<Alignment> metricAlign() override;
		const std::shared_ptr<Alignment> csmAlign() override;
		const std::shared_ptr<Alignment> fineIntAlign() override;
		std::shared_ptr<Raster<bool>> layout() override;
		std::string layoutTileName(cell_t tile) override;

		std::shared_ptr<VectorDataset<Polygon>> lasFileLayout() override;
		std::shared_ptr<VectorDataset<Polygon>> demFileLayout() override;

		std::mutex& cellMutex(cell_t cell) override;
		std::mutex& globalMutex() override;

		const std::vector<std::shared_ptr<LasFilter>>& filters() override;
		coord_t minHt() override;
		coord_t maxHt() override;
		bool overlapsAoI(const Extent& e) override;

		CsmAlgorithm* csmAlgorithm() override;
		CsmPostProcessor* csmPostProcessAlgorithm() override;

		int nThread() override;
		coord_t binSize() override;
		size_t tileFileSize() override;

		coord_t canopyCutoff() override;
		const std::vector<coord_t>& strataBreaks() override;
		const std::vector<std::string>& strataNames() override;

		TaoIdAlgorithm* taoIdAlgorithm() override;
		const std::vector<std::unique_ptr<TaoSegmentAlgorithm>>& taoSegAlgorithms() override;


		Raster<coord_t> bufferedElev(const Raster<coord_t>& unbufferedElev) override;
		const std::vector<coord_t>& topoWindows() override;
		const std::vector<std::string>& topoWindowNames() override;
		bool useRadians() override;

		const std::filesystem::path& outFolder() override;
		const std::string& name() override;
		void describeParameters(MetadataPdf& pdf) override;

		coord_t fineIntCanopyCutoff() override;

		bool doPointMetrics() override;
		bool doFirstReturnMetrics() override;
		bool doAllReturnMetrics() override;
		bool doAdvancedPointMetrics() override;
		bool doCsm() override;
		bool doCsmMetrics() override;
		bool doTaos() override;
		bool doFineInt() override;
		bool doTopo() override;
		bool doStratumMetrics() override;

		bool isDebugNoAlign() override;
		bool isDebugNoOutput() override;
		bool isAnyDebug() override;

		ParseResults parseArgs(const std::vector<std::string>& args) override;
		ParseResults parseIni(const std::string& path) override;

		std::ostream& writeOptions(std::ostream& out, ParamCategory cat) const override;

	private:

		LinearUnit _prevUnits;

		size_t _cellMutCount = 10000;
		std::unique_ptr<std::vector<std::mutex>> _cellMuts;
		std::mutex _globalMut;

		std::vector<std::string> _failedLas;

		std::shared_ptr<Raster<bool>> _layout;

		std::shared_ptr<void> _pdf;
	};


}

#endif