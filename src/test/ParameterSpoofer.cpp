#include"ParameterSpoofer.hpp"

namespace lapis {
	void SharedParameterSpoofer::setOutUnits(const Unit& u)
	{
		_outUnits = u;
	}
	const Unit& SharedParameterSpoofer::outUnits()
	{
		return _outUnits;
	}
	void SharedParameterSpoofer::setMetricAlign(const Alignment& a)
	{
		_metricAlign = std::make_shared<Alignment>(a);
	}
	const std::shared_ptr<Alignment> SharedParameterSpoofer::metricAlign()
	{
		return _metricAlign;
	}
	void SharedParameterSpoofer::setLayout(const Alignment& a)
	{
		_layout = std::make_shared<Raster<bool>>(a);
	}
	shared_raster<bool> SharedParameterSpoofer::layout()
	{
		return _layout;
	}
	std::mutex& SharedParameterSpoofer::cellMutex(cell_t cell)
	{
		return _mut;
	}
	std::mutex& SharedParameterSpoofer::globalMutex()
	{
		return _mut;
	}
	void SharedParameterSpoofer::setOutFolder(const std::filesystem::path& path)
	{
		_outFolder = path;
	}
	const std::filesystem::path& SharedParameterSpoofer::outFolder()
	{
		return _outFolder;
	}
	void SharedParameterSpoofer::setName(const std::string& name)
	{
		_name = name;
	}
	const std::string& SharedParameterSpoofer::name()
	{
		return _name;
	}
	int SharedParameterSpoofer::nThread()
	{
		return 1;
	}
	void SharedParameterSpoofer::addLasExtent(const Extent& e)
	{
		_lasExtents.push_back(e);
	}
	const std::vector<Extent>& SharedParameterSpoofer::lasExtents()
	{
		return _lasExtents;
	}
	void PointMetricParameterSpoofer::setDoPointMetrics(bool b)
	{
		_doPointMetrics = b;
	}
	bool PointMetricParameterSpoofer::doPointMetrics()
	{
		return _doPointMetrics;
	}
	void PointMetricParameterSpoofer::setDoFirstReturnMetrics(bool b)
	{
		_doFirstReturnMetrics = b;
	}
	bool PointMetricParameterSpoofer::doFirstReturnMetrics()
	{
		return _doFirstReturnMetrics;
	}
	void PointMetricParameterSpoofer::setDoAllReturnMetrics(bool b)
	{
		_doAllReturnMetrics = b;
	}
	bool PointMetricParameterSpoofer::doAllReturnMetrics()
	{
		return _doAllReturnMetrics;
	}
	void PointMetricParameterSpoofer::setDoStratumMetrics(bool b)
	{
		_doStratumMetrics = b;
	}
	bool PointMetricParameterSpoofer::doStratumMetrics()
	{
		return _doStratumMetrics;
	}
	void PointMetricParameterSpoofer::setDoAdvancedPointMetrics(bool b)
	{
		_doAdvancedPointMetrics = b;
	}
	bool PointMetricParameterSpoofer::doAdvancedPointMetrics()
	{
		return _doAdvancedPointMetrics;
	}
	void PointMetricParameterSpoofer::setCanopyCutoff(coord_t v)
	{
		_canopyCutoff = v;
	}
	coord_t PointMetricParameterSpoofer::canopyCutoff()
	{
		return _canopyCutoff;
	}
	void PointMetricParameterSpoofer::setMaxHt(coord_t v)
	{
		_maxHt = v;
	}
	coord_t PointMetricParameterSpoofer::maxHt()
	{
		return _maxHt;
	}
	coord_t PointMetricParameterSpoofer::binSize()
	{
		return convertUnits(0.01, LinearUnitDefs::meter, outUnits());
	}
	void PointMetricParameterSpoofer::setStrata(const std::vector<coord_t>& breaks, const std::vector<std::string>& names)
	{
		_strataBreaks = breaks;
		_strataNames = names;
	}
	const std::vector<coord_t>& PointMetricParameterSpoofer::strataBreaks()
	{
		return _strataBreaks;
	}
	const std::vector<std::string>& PointMetricParameterSpoofer::strataNames()
	{
		return _strataNames;
	}
	void CsmParameterSpoofer::setCsmAlign(const Alignment& a)
	{
		_csmAlign = std::make_shared<Alignment>(a);
	}
	const std::shared_ptr<Alignment> CsmParameterSpoofer::csmAlign()
	{
		return _csmAlign;
	}
	void CsmParameterSpoofer::setFootprintDiameter(coord_t v)
	{
		_footprint = v;
	}
	coord_t CsmParameterSpoofer::footprintDiameter()
	{
		return _footprint;
	}
	void CsmParameterSpoofer::setSmooth(int v)
	{
		_smooth = v;
	}
	int CsmParameterSpoofer::smooth()
	{
		return _smooth;
	}
	void CsmParameterSpoofer::setDoCsm(bool b)
	{
		_doCsm = b;
	}
	bool CsmParameterSpoofer::doCsm()
	{
		return _doCsm;
	}
	void CsmParameterSpoofer::setDoCsmMetrics(bool b)
	{
		_doCsmMetrics = b;
	}
	bool CsmParameterSpoofer::doCsmMetrics()
	{
		return _doCsmMetrics;
	}
	void TaoParameterSpoofer::setMinTaoHt(coord_t v)
	{
		_minHt = v;
	}
	coord_t TaoParameterSpoofer::minTaoHt()
	{
		return _minHt;
	}
	void TaoParameterSpoofer::setMinTaoDist(coord_t v)
	{
		_minDist = v;
	}
	coord_t TaoParameterSpoofer::minTaoDist()
	{
		return _minDist;
	}
	void TaoParameterSpoofer::setDoTaos(bool b)
	{
		_doTaos = b;
	}
	bool TaoParameterSpoofer::doTaos()
	{
		return _doTaos;
	}
	IdAlgo::IdAlgo TaoParameterSpoofer::taoIdAlgo()
	{
		return IdAlgo::HIGHPOINT;
	}
	SegAlgo::SegAlgo TaoParameterSpoofer::taoSegAlgo()
	{
		return SegAlgo::WATERSHED;
	}
	void FineIntParameterSpoofer::setFineIntAlign(const Alignment& a)
	{
		_fineIntAlign = std::make_shared<Alignment>(a);
	}
	const std::shared_ptr<Alignment> FineIntParameterSpoofer::fineIntAlign()
	{
		return _fineIntAlign;
	}
	void FineIntParameterSpoofer::setFineIntCanopyCutoff(coord_t v)
	{
		_canopy = v;
	}
	coord_t FineIntParameterSpoofer::fineIntCanopyCutoff()
	{
		return _canopy;
	}
	void FineIntParameterSpoofer::setDoFineInt(bool b)
	{
		_doFineInt = b;
	}
	bool FineIntParameterSpoofer::doFineInt()
	{
		return _doFineInt;
	}
	void TopoParameterSpoofer::setDoTopo(bool b)
	{
		_doTopo = b;
	}
	bool TopoParameterSpoofer::doTopo()
	{
		return _doTopo;
	}
}