#pragma once
#ifndef LP_RUNPARAMETERS_H
#define LP_RUNPARAMETERS_H

#include"param_pch.hpp"
#include"Parameter.hpp"
#include"ParameterGetter.hpp"

namespace lapis {

	class DemAlgorithm;

	class RunParameters : public ParameterGetter {

	public:

		static RunParameters& singleton();

		size_t registerParameter(Parameter* param);

		template<class T>
		void renderGui();

		void importBoostAndUpdateUnits();
		void updateUnits();

		bool prepareForRun();
		void cleanAfterRun();
		void resetObject();

		const Unit& outUnits();
		void setPrevUnits(const Unit& u);
		const Unit& prevUnits();
		const std::string& unitSingular();
		const std::string& unitPlural();

		const std::vector<Extent>& lasExtents();
		LasReader getLas(size_t i);

		DemAlgorithm* demAlgorithm();

		const Extent& fullExtent();
		const CoordRef& userCrs();
		const std::shared_ptr<Alignment> metricAlign();
		const std::shared_ptr<Alignment> csmAlign();
		const std::shared_ptr<Alignment> fineIntAlign();
		std::shared_ptr<Raster<bool>> layout();
		std::string layoutTileName(cell_t tile);

		std::mutex& cellMutex(cell_t cell);
		std::mutex& globalMutex();

		const std::vector<std::shared_ptr<LasFilter>>& filters();
		coord_t minHt();
		coord_t maxHt();

		CsmAlgorithm* csmAlgorithm();
		CsmPostProcessor* csmPostProcessAlgorithm();

		int nThread();
		coord_t binSize();
		size_t tileFileSize();

		coord_t canopyCutoff();
		const std::vector<coord_t>& strataBreaks();
		const std::vector<std::string>& strataNames();

		TaoIdAlgorithm* taoIdAlgorithm();
		TaoSegmentAlgorithm* taoSegAlgorithm();

		const std::filesystem::path& outFolder();
		const std::string& name();

		coord_t fineIntCanopyCutoff();

		bool doPointMetrics();
		bool doFirstReturnMetrics();
		bool doAllReturnMetrics();
		bool doAdvancedPointMetrics();
		bool doCsm();
		bool doCsmMetrics();
		bool doTaos();
		bool doFineInt();
		bool doTopo();
		bool doStratumMetrics();

		bool isDebugNoAlign();
		bool isDebugNoOutput();
		bool isAnyDebug();

		enum class ParseResults {
			invalidOpts, helpPrinted, validOpts, guiRequested
		};

		ParseResults parseArgs(const std::vector<std::string>& args);
		ParseResults parseIni(const std::string& path);

		std::ostream& writeOptions(std::ostream& out, ParamCategory cat) const;

		const int maxLapisFileName = 75;
#ifdef _WIN32
		const int maxTotalFilePath = 250;
#else
		const int maxTotalFilePath = 4000;
#endif

	private:

		friend class RunParametersTest;

		RunParameters();
		RunParameters(const RunParameters&) = delete;
		RunParameters(RunParameters&&) = delete;
		std::vector<std::unique_ptr<Parameter>> _params;

		template<class PARAMETER>
		PARAMETER& _getRawParam();

		template<class PARAMETER>
		const PARAMETER& _getRawParam() const;

		Unit _prevUnits;

		size_t _cellMutCount = 10000;
		std::unique_ptr<std::vector<std::mutex>> _cellMuts;
		std::mutex _globalMut;

		std::vector<std::string> _failedLas;

		std::shared_ptr<Raster<bool>> _layout;

		static void logGDALErrors(CPLErr eErrClass, CPLErrorNum nError, const char* pszErrorMsg);
		static void silenceGDALErrors(CPLErr eErrClass, CPLErrorNum nError, const char* pszErrorMsg) {}
		static void logProjErrors(void* v, int i, const char* c);
	};

	template<class PARAMETER>
	inline PARAMETER& RunParameters::_getRawParam()
	{
		return *dynamic_cast<PARAMETER*>(_params[PARAMETER::registeredIndex].get());
	}
	template<class PARAMETER>
	inline const PARAMETER& RunParameters::_getRawParam() const
	{
		return *dynamic_cast<PARAMETER*>(_params[PARAMETER::registeredIndex].get());
	}
	template<class T>
	void RunParameters::renderGui()
	{
		_params[T::registeredIndex]->renderGui();
	}
}

#endif