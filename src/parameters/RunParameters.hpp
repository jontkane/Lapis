#pragma once
#ifndef LP_RUNPARAMETERS_H
#define LP_RUNPARAMETERS_H

#include"param_pch.hpp"
#include"Parameter.hpp"
#include"ParameterGetter.hpp"
#include"..\utils\MetadataPdf.hpp"

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

		template<class PARAMETER>
		PARAMETER& getParam();

		template<class PARAMETER>
		const PARAMETER& getParam() const;

		const LinearUnit& outUnits();
		void setPrevUnits(const LinearUnit& u);
		const LinearUnit& prevUnits();
		const std::string& unitSingular();
		const std::string& unitPlural();

		const std::vector<Extent>& lasExtents();
		LasReader getLas(size_t i);

		std::unique_ptr<DemAlgoApplier> demAlgorithm(LasReader&& l);

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

		Raster<coord_t> bufferedElev(const Raster<coord_t>& unbufferedElev);
		const std::vector<coord_t>& topoWindows();
		const std::vector<std::string>& topoWindowNames();
		bool useRadians();

		const std::filesystem::path& outFolder();
		const std::string& name();
		//this fuction is intended to produce matadata only for parameters not associated with a single product
		void describeParameters(MetadataPdf& pdf);

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

		RunParameters();
		RunParameters(const RunParameters&) = delete;
		RunParameters(RunParameters&&) = delete;
		std::vector<std::unique_ptr<Parameter>> _params;

		LinearUnit _prevUnits;

		size_t _cellMutCount = 10000;
		std::unique_ptr<std::vector<std::mutex>> _cellMuts;
		std::mutex _globalMut;

		std::vector<std::string> _failedLas;

		std::shared_ptr<Raster<bool>> _layout;

		std::shared_ptr<void> _pdf;
	};

	template<class PARAMETER>
	inline PARAMETER& RunParameters::getParam()
	{
		return *dynamic_cast<PARAMETER*>(_params[PARAMETER::parameterRegisteredIndex].get());
	}
	template<class PARAMETER>
	inline const PARAMETER& RunParameters::getParam() const
	{
		return *dynamic_cast<PARAMETER*>(_params[PARAMETER::parameterRegisteredIndex].get());
	}
	template<class T>
	void RunParameters::renderGui()
	{
		_params[T::parameterRegisteredIndex]->renderGui();
	}
}

#endif