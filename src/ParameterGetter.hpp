#pragma once
#ifndef LP_PARAMETERGETTER_H
#define LP_PARAMETERGETTER_H

#include"app_pch.hpp"
#include"LapisTypeDefs.hpp"
#include"LapisUtils.hpp"

//This file defines a number of abstract classes designed to be interfaces for the classes in ProductHandler.hpp
//In the production code, the only class inheriting from any of them will be LapisData, but other implementation will be used for data injection in tests
namespace lapis {

	class SharedParameterGetter {
	public:
		virtual const Unit& outUnits()= 0;
		virtual const std::shared_ptr<Alignment> metricAlign() = 0;
		virtual shared_raster<bool> layout() = 0;
		virtual std::mutex& cellMutex(cell_t cell) = 0;
		virtual std::mutex& globalMutex() = 0;
		virtual const std::filesystem::path& outFolder() = 0;
		virtual const std::string& name() = 0;
		virtual int nThread() = 0;
		virtual const std::vector<Extent>& lasExtents() = 0;
	};

	class PointMetricParameterGetter : public virtual SharedParameterGetter {
	public:
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
		virtual const std::shared_ptr<Alignment> csmAlign() = 0;
		virtual coord_t footprintDiameter() = 0;
		virtual int smooth() = 0;
		virtual bool doCsm() = 0;
		virtual bool doCsmMetrics() = 0;
	};

	class TaoParameterGetter : public virtual SharedParameterGetter {
	public:
		virtual coord_t minTaoHt() = 0;
		virtual coord_t minTaoDist() = 0;
		virtual bool doTaos() = 0;
		virtual IdAlgo::IdAlgo taoIdAlgo() = 0;
		virtual SegAlgo::SegAlgo taoSegAlgo()= 0;
	};

	class FineIntParameterGetter : public virtual SharedParameterGetter {
	public:
		virtual const std::shared_ptr<Alignment> fineIntAlign() = 0;
		virtual coord_t fineIntCanopyCutoff() = 0;
		virtual bool doFineInt() = 0;
	};

	class TopoParameterGetter : public virtual SharedParameterGetter {
	public:
		virtual bool doTopo() = 0;
	};

	class ParameterGetter :
		public PointMetricParameterGetter,
		public CsmParameterGetter,
		public TaoParameterGetter,
		public FineIntParameterGetter,
		public TopoParameterGetter
	{};
}

#endif