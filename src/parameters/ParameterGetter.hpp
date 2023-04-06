#pragma once
#ifndef LP_PARAMETERGETTER_H
#define LP_PARAMETERGETTER_H

#include"param_pch.hpp"
#include"..\algorithms\AllAlgorithmTypes.hpp"

//This file defines a number of abstract classes designed to be interfaces for the classes in ProductHandler.hpp
//In the production code, the only class inheriting from any of them will be LapisData, but other implementation will be used for data injection in tests
namespace lapis {


	class SharedParameterGetter {
	public:

		virtual ~SharedParameterGetter() = default;

		virtual const LinearUnit& outUnits() = 0;
		virtual const std::string& unitSingular() = 0;
		virtual const std::string& unitPlural() = 0;
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
		virtual TaoSegmentAlgorithm* taoSegAlgorithm() = 0;
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
	};

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
}

#endif