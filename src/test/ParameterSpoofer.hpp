#pragma once
#ifndef LP_PARAMETERSPOOFER_H
#define LP_PARAMETERSPOOFER_H

#include"..\ParameterGetter.hpp"

namespace lapis {
	
#pragma warning(push)
#pragma warning(disable : 4250) //as far as I can tell, this warning always occurs with diamond inheritance

	class SharedParameterSpoofer : public virtual SharedParameterGetter {
	public:
		void setOutUnits(const Unit& u);
		const Unit& outUnits();

		void setMetricAlign(const Alignment& a);
		const std::shared_ptr<Alignment> metricAlign();

		void setLayout(const Alignment& a);
		shared_raster<bool> layout();


		std::mutex& cellMutex(cell_t cell);
		std::mutex& globalMutex();

		void setOutFolder(const std::filesystem::path& path);
		const std::filesystem::path& outFolder();

		void setName(const std::string& name);
		const std::string& name();

		int nThread();

		void addLasExtent(const Extent& e);
		const std::vector<Extent>& lasExtents();

	private:
		Unit _outUnits;
		std::shared_ptr<Alignment> _metricAlign;
		shared_raster<bool> _layout;
		std::filesystem::path _outFolder;
		std::string _name;
		std::vector<Extent> _lasExtents;
		std::mutex _mut;
	};

	inline void setReasonableSharedDefaults(SharedParameterSpoofer& spoof) {
		spoof.setOutUnits(LinearUnitDefs::meter);
		spoof.setName("TEST");
		spoof.setOutFolder(std::string(LAPISTESTFILES) + "/output/");
		spoof.setMetricAlign(Alignment(Extent(0, 3, 0, 3), 3, 3));
		spoof.setLayout(Alignment(Extent(0, 4, 0, 4), 2, 2));
	}


	class PointMetricParameterSpoofer : public virtual PointMetricParameterGetter, public SharedParameterSpoofer {
	public:
		void setDoPointMetrics(bool b);
		bool doPointMetrics();

		void setDoFirstReturnMetrics(bool b);
		bool doFirstReturnMetrics();

		void setDoAllReturnMetrics(bool b);
		bool doAllReturnMetrics();

		void setDoStratumMetrics(bool b);
		bool doStratumMetrics();

		void setDoAdvancedPointMetrics(bool b);
		bool doAdvancedPointMetrics();

		void setCanopyCutoff(coord_t v);
		coord_t canopyCutoff();

		void setMaxHt(coord_t v);
		coord_t maxHt();

		coord_t binSize();

		void setStrata(const std::vector<coord_t>& breaks, const std::vector<std::string>& names);
		const std::vector<coord_t>& strataBreaks();
		const std::vector<std::string>& strataNames();

	private:
		bool _doPointMetrics = true;
		bool _doFirstReturnMetrics = true;
		bool _doAllReturnMetrics = true;
		bool _doStratumMetrics = true;
		bool _doAdvancedPointMetrics = true;

		coord_t _canopyCutoff = 2;

		coord_t _maxHt = 100;

		std::vector<coord_t> _strataBreaks;
		std::vector<std::string> _strataNames;
	};

	class CsmParameterSpoofer : public virtual CsmParameterGetter, public SharedParameterSpoofer {
	public:
		void setCsmAlign(const Alignment& a);
		const std::shared_ptr<Alignment> csmAlign();

		void setFootprintDiameter(coord_t v);
		coord_t footprintDiameter();

		void setSmooth(int v);
		int smooth();

		void setDoCsm(bool b);
		bool doCsm();

		void setDoCsmMetrics(bool b);
		bool doCsmMetrics();

	private:
		std::shared_ptr<Alignment> _csmAlign;
		coord_t _footprint = 0.4;
		int _smooth = 3;
		bool _doCsm = true;
		bool _doCsmMetrics = true;
	};

	class TaoParameterSpoofer : public virtual TaoParameterGetter, public SharedParameterSpoofer {
	public:

		void setMinTaoHt(coord_t v);
		coord_t minTaoHt();

		void setMinTaoDist(coord_t v);
		coord_t minTaoDist();

		void setDoTaos(bool b);
		bool doTaos();


		IdAlgo::IdAlgo taoIdAlgo();
		SegAlgo::SegAlgo taoSegAlgo();

	private:
		coord_t _minHt = 2;
		coord_t _minDist = 0;
		bool _doTaos = true;
	};

	class FineIntParameterSpoofer : public virtual FineIntParameterGetter, public SharedParameterSpoofer {
	public:

		void setFineIntAlign(const Alignment& a);
		const std::shared_ptr<Alignment> fineIntAlign();

		void setFineIntCanopyCutoff(coord_t v);
		coord_t fineIntCanopyCutoff();

		void setDoFineInt(bool b);
		bool doFineInt();

	private:
		std::shared_ptr<Alignment> _fineIntAlign;
		coord_t _canopy = 2;
		bool _doFineInt = true;
	};

	class TopoParameterSpoofer : public virtual TopoParameterGetter, public SharedParameterSpoofer {
	public:
		void setDoTopo(bool b);
		bool doTopo();

	private:
		bool _doTopo = true;
	};

#pragma warning(pop)

}

#endif