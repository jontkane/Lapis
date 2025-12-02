#pragma once
#ifndef LP_PARAMETERGETTER_H
#define LP_PARAMETERGETTER_H

#include"param_pch.hpp"
#include"..\algorithms\AllAlgorithmTypes.hpp"
#include"Parameter.hpp"

//This file defines a number of abstract classes designed to be interfaces for the classes in ProductHandler.hpp
//In the production code, the only class inheriting from any of them will be LapisParameters, but other implementation will be used for data injection in tests
namespace lapis {


	class SharedParameterGetter {
	public:

		virtual ~SharedParameterGetter() = default;

		virtual const LinearUnit& outUnits() = 0;
		virtual const std::string& unitSingular() = 0;
		virtual const std::string& unitPlural() = 0;
		virtual const Extent& fullExtent() = 0;
		virtual const std::shared_ptr<Alignment> metricAlign() = 0;
		virtual std::shared_ptr<Raster<bool>> layout() = 0;
		virtual std::mutex& cellMutex(cell_t cell) = 0;
		virtual std::mutex& globalMutex() = 0;
		virtual const std::filesystem::path& outFolder() = 0;
		virtual const std::string& name() = 0;
		virtual int nThread() = 0;
		virtual const std::vector<Extent>& lasExtents() = 0;
		virtual std::string layoutTileName(cell_t tile) = 0;
	};

	class PointMetricParameterGetter : public virtual SharedParameterGetter {
	public:

		virtual ~PointMetricParameterGetter() = default;

		virtual bool doPointMetrics() = 0;
		virtual bool doFirstReturnMetrics() = 0;
		virtual bool doAllReturnMetrics() = 0;
		virtual bool doStratumMetrics() = 0;
		virtual bool doAdvancedPointMetrics() = 0;
		virtual coord_t canopyCutoff() = 0;
        virtual coord_t minHt() = 0;
		virtual coord_t maxHt() = 0;
		virtual coord_t binSize() = 0;
		virtual const std::vector<coord_t>& strataBreaks() = 0;
		virtual const std::vector<std::string>& strataNames() = 0;
	};

	class CsmParameterGetter : public virtual SharedParameterGetter {
	public:

		virtual ~CsmParameterGetter() = default;

		virtual const std::shared_ptr<Alignment> csmAlign() = 0;
		virtual CsmAlgorithm* csmAlgorithm() = 0;
		virtual CsmPostProcessor* csmPostProcessAlgorithm() = 0;
		virtual bool doCsm() = 0;
		virtual bool doCsmMetrics() = 0;
	};

	class TaoParameterGetter : public virtual SharedParameterGetter {
	public:
		virtual ~TaoParameterGetter() = default;

		virtual TaoIdAlgorithm* taoIdAlgorithm() = 0;
		virtual const std::vector<std::unique_ptr<TaoSegmentAlgorithm>>& taoSegAlgorithms() = 0;
		virtual bool doTaos() = 0;
	};

	class FineIntParameterGetter : public virtual SharedParameterGetter {
	public:
		virtual ~FineIntParameterGetter() = default;

		virtual const std::shared_ptr<Alignment> fineIntAlign() = 0;
		virtual coord_t fineIntCanopyCutoff() = 0;
		virtual bool doFineInt() = 0;
	};

	class TopoParameterGetter : public virtual SharedParameterGetter {
	public:
		virtual ~TopoParameterGetter() = default;

		virtual bool doTopo() = 0;
		virtual Raster<coord_t> bufferedElev(const Raster<coord_t>& unbufferedElev) = 0;
		virtual const std::vector<coord_t>& topoWindows() = 0;
		virtual const std::vector<std::string>& topoWindowNames() = 0;
		virtual bool useRadians() = 0;
	};

	//this class should contain all the functions necessary simply for accessing parameters
	class ParameterGetter :
		public PointMetricParameterGetter,
		public CsmParameterGetter,
		public TaoParameterGetter,
		public FineIntParameterGetter,
		public TopoParameterGetter
	{
	public:
		virtual ~ParameterGetter() = default;
	};

	//this class contains the additional functions necessary for managing parameters,
    //such as updating their units, rendering the gui, etc
    //these functions are not intended to be used by the product handlers, but rather by the main program
	class ParameterManager : public ParameterGetter {
	public:
        virtual ~ParameterManager() = default;

		virtual bool prepareForRun() = 0;
		virtual void cleanAfterRun() = 0;
		virtual void reset() = 0;

		template<class PARAMETER>
		PARAMETER& getParam();

		template<class PARAMETER>
		const PARAMETER& getParam() const;

		template<class T>
		void renderGui();

		virtual void importBoostAndUpdateUnits() = 0;
		virtual void updateUnits() = 0;
		virtual void setPrevUnits(const LinearUnit& u) = 0;
		virtual const LinearUnit& prevUnits() = 0;

		virtual LasReader getLas(size_t i) = 0;
		virtual std::optional<LinearUnit> lasZUnits() = 0;

        virtual bool demExists() = 0;
		virtual std::unique_ptr<DemAlgoApplier> demAlgorithm(LasReader&& l) = 0;

		//userCrsSpecification() returns what the user actually selected. It may be an empty crs, indicating no particular preference
		virtual const CoordRef& userCrsSpecification() = 0;
		//outputCrs() returns the actual crs being used. It will only be empty in rare circumstances
		virtual const CoordRef& outputCrs() = 0;

		virtual std::shared_ptr<VectorDataset<Polygon>> lasFileLayout() = 0;
		virtual std::shared_ptr<VectorDataset<Polygon>> demFileLayout() = 0;

		virtual const std::vector<std::shared_ptr<LasFilter>>& filters() = 0;
		virtual bool overlapsAoI(const Extent& e) = 0;

		virtual size_t tileFileSize() = 0;

		//this fuction is intended to produce matadata only for parameters not associated with a single product
		virtual void describeParameters(MetadataPdf& pdf) = 0;

		enum class ParseResults {
			invalidOpts, helpPrinted, validOpts, guiRequested
		};
		virtual ParseResults parseArgs(const std::vector<std::string>& args) = 0;
		virtual ParseResults parseIni(const std::string& path) = 0;

		virtual std::ostream& writeOptions(std::ostream& out, ParamCategory cat) const = 0;

		const int maxLapisFileName = 75;
#ifdef _WIN32
		const int maxTotalFilePath = 250;
#else
		const int maxTotalFilePath = 4000;
#endif
	};

    void setParameterManager(ParameterManager* pm);
    ParameterManager& parameterManager();

	template<class PARAMETER>
	inline PARAMETER& ParameterManager::getParam()
	{
        return *ParameterRegistrar::get().getParameter<PARAMETER>();
	}
	template<class PARAMETER>
	inline const PARAMETER& ParameterManager::getParam() const
	{
        return *ParameterRegistrar::get().getParameter<PARAMETER>();
	}
	template<class T>
	void ParameterManager::renderGui()
	{
        getParam<T>().renderGui();
	}
}

#endif