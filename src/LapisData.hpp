#pragma once
#ifndef LAPISDATA_H
#define LAPISDATA_H

#include"app_pch.hpp"
#include"LapisParameters.hpp"
#include"LapisUtils.hpp"
#include"MetricTypeDefs.hpp"

namespace lapis {

	template<class T>
	using shared_raster = std::shared_ptr<Raster<T>>;

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
		void importBoostAndUpdateUnits();

		void prepareForRun();
		void cleanAfterRun();
		void resetObject();

		std::shared_ptr<Alignment> metricAlign();
		std::shared_ptr<Alignment> csmAlign();

		
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
		size_t csmFileSize();

		coord_t canopyCutoff();
		const std::vector<coord_t>& strataBreaks();

		const std::filesystem::path& outFolder();
		const std::string& name();

		bool doFirstReturnMetrics();
		bool doAllReturnMetrics();
		bool doCsm();
		bool doTaos();
		bool doFineInt();
		bool doTopo();
		bool doAdvPointMetrics();

		enum class ParseResults {
			invalidOpts, helpPrinted, validOpts, guiRequested
		};

		ParseResults parseArgs(const std::vector<std::string>& args);
		ParseResults parseIni(const std::string& path);

		std::ostream& writeOptions(std::ostream& out, ParamCategory cat);

		static void silenceGDALErrors(CPLErr eErrClass, CPLErrorNum nError, const char* pszErrorMsg) {}

		double estimateMemory();

		double estimateHardDrive();

		bool needAbort;


	private:

		template<ParamName N>
		friend class Parameter;

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

		const int maxLapisFileName = 75;
#ifdef _WIN32
		const int maxTotalFilePath = 250;
#else
		const int maxTotalFilePath = 4000;
#endif
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