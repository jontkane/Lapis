#pragma once
#ifndef LP_PARAMETERSPOOFER_H
#define LP_PARAMETERSPOOFER_H

#include"..\parameters\ParameterGetter.hpp"

namespace lapis {
	
#pragma warning(push)
#pragma warning(disable : 4250) //as far as I can tell, this warning always occurs with diamond inheritance

	class SharedParameterSpoofer : public virtual SharedParameterGetter {
	public:
		void setOutUnits(const Unit& u);
		const Unit& outUnits() override;

		void setMetricAlign(const Alignment& a);
		const std::shared_ptr<Alignment> metricAlign() override;

		void setLayout(const Alignment& a);
		std::shared_ptr<Raster<bool>> layout() override;


		std::mutex& cellMutex(cell_t cell);
		std::mutex& globalMutex() override;

		void setOutFolder(const std::filesystem::path& path);
		const std::filesystem::path& outFolder() override;

		void setName(const std::string& name);
		const std::string& name() override;

		int nThread() override;

		void addLasExtent(const Extent& e);
		const std::vector<Extent>& lasExtents() override;

		std::string layoutTileName(cell_t tile) override;

	private:
		Unit _outUnits;
		std::shared_ptr<Alignment> _metricAlign;
		std::shared_ptr<Raster<bool>> _layout;
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
		bool doPointMetrics() override;

		void setDoFirstReturnMetrics(bool b);
		bool doFirstReturnMetrics() override;

		void setDoAllReturnMetrics(bool b);
		bool doAllReturnMetrics() override;

		void setDoStratumMetrics(bool b);
		bool doStratumMetrics() override;

		void setDoAdvancedPointMetrics(bool b);
		bool doAdvancedPointMetrics() override;

		void setCanopyCutoff(coord_t v);
		coord_t canopyCutoff() override;

		void setMaxHt(coord_t v);
		coord_t maxHt() override;

		coord_t binSize() override;

		void setStrata(const std::vector<coord_t>& breaks, const std::vector<std::string>& names);
		const std::vector<coord_t>& strataBreaks() override;
		const std::vector<std::string>& strataNames() override;

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
		const std::shared_ptr<Alignment> csmAlign() override;

		void setCsmAlgo(CsmAlgorithm* algo);
		CsmAlgorithm* csmAlgorithm() override;

		void setCsmPostProcessor(CsmPostProcessor* algo);
		CsmPostProcessor* csmPostProcessAlgorithm() override;

		void setDoCsm(bool b);
		bool doCsm() override;

		void setDoCsmMetrics(bool b);
		bool doCsmMetrics() override;

	private:
		std::shared_ptr<Alignment> _csmAlign;
		bool _doCsm = true;
		bool _doCsmMetrics = true;

		std::unique_ptr<CsmAlgorithm> _csmAlgorithm;
		std::unique_ptr<CsmPostProcessor> _csmPostProcessAlgorithm;
	};

	class TaoParameterSpoofer : public virtual TaoParameterGetter, public SharedParameterSpoofer {
	public:

		void setDoTaos(bool b);
		bool doTaos() override;


		void setTaoIdAlgorithm(TaoIdAlgorithm* algo);
		TaoIdAlgorithm* taoIdAlgorithm() override;

		void setTaoSegAlgorithm(TaoSegmentAlgorithm* algo);
		TaoSegmentAlgorithm* taoSegAlgorithm() override;

	private:
		std::unique_ptr<TaoIdAlgorithm> _taoIdAlgorithm;
		std::unique_ptr<TaoSegmentAlgorithm> _taoSegAlgorithm;
		bool _doTaos = true;
	};

	class FineIntParameterSpoofer : public virtual FineIntParameterGetter, public SharedParameterSpoofer {
	public:

		void setFineIntAlign(const Alignment& a);
		const std::shared_ptr<Alignment> fineIntAlign() override;

		void setFineIntCanopyCutoff(coord_t v);
		coord_t fineIntCanopyCutoff() override;

		void setDoFineInt(bool b);
		bool doFineInt() override;

	private:
		std::shared_ptr<Alignment> _fineIntAlign;
		coord_t _canopy = 2;
		bool _doFineInt = true;
	};

	class TopoParameterSpoofer : public virtual TopoParameterGetter, public SharedParameterSpoofer {
	public:
		void setDoTopo(bool b);
		bool doTopo() override;

	private:
		bool _doTopo = true;
	};

#pragma warning(pop)

}

#endif