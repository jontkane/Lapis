#include"param_pch.hpp"
#include"CsmParameter.hpp"
#include"RunParameters.hpp"
#include"..\algorithms\AllCsmAlgorithms.hpp"
#include"..\algorithms\AllCsmPostProcessors.hpp"

namespace lapis {

	size_t CsmParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new CsmParameter());
	void CsmParameter::reset()
	{
		*this = CsmParameter();
	}

	CsmParameter::CsmParameter() {
		_smooth.addOption("3x3", 3, 3);
		_smooth.addOption("No Smoothing", 1, 1);
		_smooth.addOption("5x5", 5, 5);
		_smooth.setSingleLine();

		_footprintDiameter.addHelpText("This value indicates the estimated diameter of a lidar pulse as it strikes the canopy.\n\n"
			"Each return will be considered a circle with this diameter for the purpose of constructing the canopy surface model.\n\n"
			"If this behavior is undesirable, set the value to 0.");
		_smooth.addHelpText("The desired smoothing for the output CSM.\n\n"
			"If set to 3x3, each cell in the final raster will be equal to the average of that cell and its 8 neighbors in the pre-smoothing CSM.\n\n"
			"If set to 5x5, it will use the average of those 9 cells and the 16 cells surrounding them.");
		_doMetrics.addHelpText("If this is checked, Lapis will calculate summary metrics on the CSM itself, such as the mean and standard deviation of values.");
		_fill.addHelpText("It's somewhat common, especially in low-density lidar flights, for there to be small areas where a CSM cannot be calculated.\n"
			"If this is checked, those areas will be filled by interpolating from nearby areas.\n");
	}
	void CsmParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_cellsize.addToCmd(visible, hidden);
		_footprintDiameter.addToCmd(visible, hidden);
		_doMetrics.addToCmd(visible, hidden);
		_smooth.addToCmd(visible, hidden);
		_fill.addToCmd(visible, hidden);
	}
	std::ostream& CsmParameter::printToIni(std::ostream& o) {
		_cellsize.printToIni(o);
		_footprintDiameter.printToIni(o);
		_doMetrics.printToIni(o);
		_smooth.printToIni(o);
		_fill.printToIni(o);
		return o;
	}
	ParamCategory CsmParameter::getCategory() const {
		return ParamCategory::process;
	}
	void CsmParameter::renderGui() {
		_title.renderGui();

		_cellsize.renderGui();

		_smooth.renderGui();
		_footprintDiameter.renderGui();
		_fill.renderGui();

		_doMetrics.renderGui();
	}
	void CsmParameter::updateUnits() {
		_cellsize.updateUnits();
		_footprintDiameter.updateUnits();
	}
	void CsmParameter::importFromBoost() {
		_smooth.importFromBoost();
		_footprintDiameter.importFromBoost();
		_cellsize.importFromBoost();
		_doMetrics.importFromBoost();
		_fill.importFromBoost();
	}
	bool CsmParameter::prepareForRun() {
		if (_runPrepared) {
			return true;
		}

		RunParameters& rp = RunParameters::singleton();
		LapisLogger& log = LapisLogger::getLogger();
		const Alignment& metricAlign = *rp.metricAlign();
		const LinearUnit& u = rp.outUnits();


		if (!rp.doCsm()) {
			//there's a few random places in the code that use the csm alignment even if the csm isn't being calculated
			//if the csm is turned off, so we don't have its parameters, just default to 1 meter cellsize
			coord_t cellsize = linearUnitPresets::meter.convertOneFromThis(1, metricAlign.crs().getXYLinearUnits());
			_csmAlign = std::make_shared<Alignment>(metricAlign, metricAlign.xOrigin(), metricAlign.yOrigin(), cellsize, cellsize);
			_runPrepared = true;
			return true;
		}

		if (std::isnan(_footprintDiameter.getValueLogErrors()) || std::isnan(_cellsize.getValueLogErrors())) {
			return false;
		}

		if (_footprintDiameter.getValueLogErrors() < 0) {
			log.logMessage("Pulse diameter must be non-negative");
			return false;
		}
		if (_cellsize.getValueLogErrors() <= 0) {
			log.logMessage("CSM Cellsize must be positive");
			return false;
		}

		//so large that the outer edge of the circle will skip entire cells sometimes
		if (_footprintDiameter.getValueLogErrors() >= _cellsize.getValueLogErrors() * 2) {
			log.logMessage("Pulse diameter is too large for your CSM resolution. Increase CSM cellsize or reduce pulse diameter.");
		}

		coord_t cellsize = convertUnits(_cellsize.getValueLogErrors(), u, metricAlign.crs().getXYUnits());
		Alignment csmAlign{ metricAlign, metricAlign.xOrigin(), metricAlign.yOrigin(), cellsize, cellsize };
		coord_t footprintRadius = _footprintDiameter.getValueLogErrors() / 2.;
		csmAlign = extendAlignment(csmAlign,
			Extent(csmAlign.xmin() - footprintRadius,
				csmAlign.xmax() + footprintRadius,
				csmAlign.ymin() - footprintRadius,
				csmAlign.ymax() + footprintRadius),
			SnapType::out);
		_csmAlign = std::make_shared<Alignment>(csmAlign);

		_csmAlgorithm = std::make_unique<MaxPoint>(_footprintDiameter.getValueLogErrors());

		coord_t lookDist = convertUnits(5, linearUnitPresets::meter, _csmAlign->crs().getXYUnits());
		if (_smooth.currentSelection() > 1 && _fill.currentState()) {
			_csmPostProcessor = std::make_unique<SmoothAndFill>(_smooth.currentSelection(), 6, lookDist);
		}
		else if (_smooth.currentSelection() > 1) {
			_csmPostProcessor = std::make_unique<SmoothCsm>(_smooth.currentSelection());
		}
		else if (_fill.currentState()) {
			_csmPostProcessor = std::make_unique<FillCsm>(6, lookDist);
		}
		else {
			_csmPostProcessor = std::make_unique<DoNothingCsm>();
		}

		_runPrepared = true;
		return true;
	}
	void CsmParameter::cleanAfterRun() {
		_csmAlign.reset();
		_csmAlgorithm.reset();
		_csmPostProcessor.reset();
		_runPrepared = false;
	}
	std::shared_ptr<Alignment> CsmParameter::csmAlign()
	{
		prepareForRun();
		return _csmAlign;
	}
	bool CsmParameter::doCsmMetrics() const
	{
		return _doMetrics.currentState();
	}
	CsmAlgorithm* CsmParameter::csmAlgorithm()
	{
		prepareForRun();
		return _csmAlgorithm.get();
	}
	CsmPostProcessor* CsmParameter::csmPostProcessor()
	{
		prepareForRun();
		return _csmPostProcessor.get();
	}
	int CsmParameter::SmoothDecider::operator()(const std::string& s) const
	{
		int v = -1;
		try {
			v = std::stoi(s);
		}
		catch (std::invalid_argument e) {}
		return v;
	}
	std::string CsmParameter::SmoothDecider::operator()(int i) const
	{
		return std::to_string(i);
	}
}