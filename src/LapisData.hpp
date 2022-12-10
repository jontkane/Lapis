#pragma once
#ifndef LAPISDATA_H
#define LAPISDATA_H

#include"app_pch.hpp"
#include"ParameterBase.hpp"
#include"LapisUtils.hpp"
#include"MetricTypeDefs.hpp"

namespace lapis {

	class LapisData {

	public:

		static LapisData& getDataObject();

		struct AlignWithoutExtent {
			CoordRef crs;
			coord_t xres, yres, xorigin, yorigin;
		};
		struct ClassFilter {
			bool isWhiteList;
			std::unordered_set<uint8_t> list;
		};

		void renderGui(ParamName pn);
		void setPrevUnits(const Unit& u);
		const Unit& getCurrentUnits() const;
		const Unit& getPrevUnits() const;
		const std::string& getUnitSingular() const;
		const std::string& getUnitPlural() const;
		void importBoostAndUpdateUnits();

		void updateUnits();

		void prepareForRun();
		void cleanAfterRun();
		void resetObject();

		Extent fullExtent();
		const CoordRef& userCrs();
		std::shared_ptr<Alignment> metricAlign();
		std::shared_ptr<Alignment> csmAlign();
		std::shared_ptr<Alignment> fineIntAlign();

		
		shared_raster<int> nLazRaster();
		shared_raster<PointMetricCalculator> allReturnPMC();
		shared_raster<PointMetricCalculator> firstReturnPMC();

		std::mutex& cellMutex(cell_t cell);
		std::mutex& globalMutex();

		std::vector<PointMetricRaster>& allReturnPointMetrics();
		std::vector<PointMetricRaster>& firstReturnPointMetrics();
		std::vector<StratumMetricRasters>& allReturnStratumMetrics();
		std::vector<StratumMetricRasters>& firstReturnStratumMetrics();
		std::vector<CSMMetric>& csmMetrics();
		std::vector<TopoMetric>& topoMetrics();
		shared_raster<coord_t> elevNum();
		shared_raster<coord_t> elevDenom();

		const std::vector<std::shared_ptr<LasFilter>>& filters();
		coord_t minHt();
		coord_t maxHt();

		const CoordRef& lasCrsOverride();
		const CoordRef& demCrsOverride();
		const Unit& lasUnitOverride();
		const Unit& demUnitOverride();

		coord_t footprintDiameter();
		int smooth();

		const std::vector<LasFileExtent>& sortedLasList();
		const std::set<DemFileAlignment>& demList();

		int nThread();
		coord_t binSize();
		size_t tileFileSize();

		coord_t canopyCutoff();
		coord_t minTaoHt();
		coord_t minTaoDist();
		const std::vector<coord_t>& strataBreaks();

		std::filesystem::path outFolder();
		std::string name();

		coord_t fineIntCanopyCutoff();

		bool doPointMetrics();
		bool doFirstReturnMetrics();
		bool doAllReturnMetrics();
		bool doCsm();
		bool doTaos();
		bool doFineInt();
		bool doTopo();
		bool doStratumMetrics();

		bool isDebugNoAlloc();
		bool isDebugNoAlign();
		bool isDebugNoOutput();
		bool isAnyDebug();

		enum class ParseResults {
			invalidOpts, helpPrinted, validOpts, guiRequested
		};

		ParseResults parseArgs(const std::vector<std::string>& args);
		ParseResults parseIni(const std::string& path);

		std::ostream& writeOptions(std::ostream& out, ParamCategory cat);

		static void silenceGDALErrors(CPLErr eErrClass, CPLErrorNum nError, const char* pszErrorMsg) {}
		static void logGDALErrors(CPLErr eErrClass, CPLErrorNum nError, const char* pszErrorMsg);
		static void logProjErrors(void* v, int i, const char* c);

		double estimateMemory();

		double estimateHardDrive();

		struct DataIssues {
			std::vector<std::string> failedLas;
			std::vector<std::string> successfulLas;
			std::map<uint16_t, std::set<size_t>> byFileYear;
			using CrsToIdx = std::unordered_map<CoordRef, std::set<size_t>, CoordRefHasher, CoordRefComparator>;
			CrsToIdx byCrs;

			std::vector<std::string> successfulDem;
			CrsToIdx demByCrs;

			cell_t cellsInLas = 0;
			cell_t cellsInDem = 0;

			coord_t totalArea = 0;
			coord_t overlapArea = 0;

			size_t pointsInSample = 0;
			size_t pointsAfterFilters = 0;
			size_t pointsAfterDem = 0;
		};

		DataIssues checkForDataIssues();

		bool needAbort;

		void reportFailedLas(const std::string& s);

		const int maxLapisFileName = 75;
#ifdef _WIN32
		const int maxTotalFilePath = 250;
#else
		const int maxTotalFilePath = 4000;
#endif

	private:

		LapisData();
		std::vector<std::unique_ptr<ParamBase>> _params;

		//template trickery to get a loop over ParamName values recursively
		template<ParamName N>
		void _addParam();

		template<ParamName N>
		Parameter<N>& _getRawParam();

		template<ParamName N>
		const Parameter<N>& _getRawParam() const;

		Unit _prevUnits;

		size_t _cellMutCount = 10000;
		std::unique_ptr<std::vector<std::mutex>> _cellMuts;
		std::mutex _globalMut;

		std::vector<std::string> _failedLas;

		void _checkLasDemOverlap(DataIssues& di, const std::set<LasFileExtent>& las, const std::set<DemFileAlignment>& dem);

		void _checkSampleLas(DataIssues& di, const std::string& filename, const std::set<DemFileAlignment>& demSet);
	};

	template<ParamName N>
	inline void LapisData::_addParam()
	{
		if constexpr(static_cast<int>(N) >= static_cast<int>(ParamName::_Count_NotAParam)) {
			return;
		}
		else {
			constexpr ParamName NEXT = (ParamName)((int)N + 1);
			_params.emplace_back(std::make_unique<Parameter<N>>());
			_addParam<NEXT>();
		}
	}

	template<ParamName N>
	inline Parameter<N>& LapisData::_getRawParam()
	{
		return *dynamic_cast<Parameter<N>*>(_params[(size_t)N].get());
	}
	template<ParamName N>
	inline const Parameter<N>& LapisData::_getRawParam() const
	{
		return *dynamic_cast<Parameter<N>*>(_params[(size_t)N].get());
	}
}

#endif