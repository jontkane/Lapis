#include"param_pch.hpp"
#include"TaoParameter.hpp"
#include"RunParameters.hpp"
#include"..\algorithms\AllTaoIdAlgorithms.hpp"
#include"..\algorithms\AllTaoSegmentAlgorithms.hpp"

namespace lapis {

	size_t TaoParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new TaoParameter());
	void TaoParameter::reset()
	{
		*this = TaoParameter();
	}

	TaoParameter::TaoParameter() {
		_idAlgo.addOption("High Points", IdAlgo::HIGHPOINT, IdAlgo::HIGHPOINT);
		_idAlgo.setSingleLine();
		_segAlgo.addOption("Watershed", SegAlgo::WATERSHED, SegAlgo::WATERSHED);
		_segAlgo.setSingleLine();

		_sameMinHt.setState(true);

		_idAlgo.addHelpText("The algorithm for identifying trees. Only the 'high points' algorithm is supported currently.\n\n"
			"High Points: This is a CSM-based algorith. A CSM pixel is considered a candidate for being the stem of a tree if it's higher than all 8 of its neighbors.\n"
			"It is good for trees such as conifers with a well-defined tops.");
		_segAlgo.addHelpText("The algorithm for segmenting the canopy between the identified trees. Only the watershed algorith is supported currently.\n\n"
			"Watershed: expand each identified tree downwards from the center until it would have to go upwards again.\n"
			"The watershed algorithm requires no parameterization");

		_mindist.addHelpText("If two TAOs are very close to each other, it may represent an error rather than two separate trees. "
			"If this is set to a value greater than 0, then if two TAOs are too close, the shorter one will be removed.");

	}
	void TaoParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_minht.addToCmd(visible, hidden);
		_mindist.addToCmd(visible, hidden);
		_idAlgo.addToCmd(visible, hidden);
		_segAlgo.addToCmd(visible, hidden);
	}
	std::ostream& TaoParameter::printToIni(std::ostream& o) {
		if (!_sameMinHt.currentState()) {
			_minht.printToIni(o);
		}
		_mindist.printToIni(o);

		_idAlgo.printToIni(o);
		_segAlgo.printToIni(o);
		return o;
	}
	ParamCategory TaoParameter::getCategory() const {
		return ParamCategory::process;
	}
	void TaoParameter::renderGui() {
		if (!RunParameters::singleton().doCsm()) {
			ImGui::Text("Tree identification requires a CSM");
			return;
		}

		_idAlgo.renderGui();
		_segAlgo.renderGui();

		_mindist.renderGui();

		ImGui::Text("Minimum Tree Height:");
		_sameMinHt.renderGui();
		ImGui::SameLine();
		if (_sameMinHt.currentState()) {
			ImGui::BeginDisabled();
		}
		_minht.renderGui();
		if (_sameMinHt.currentState()) {
			ImGui::EndDisabled();
		}
	}
	void TaoParameter::importFromBoost() {

		//the first time this function is called, the default value for minHt will be copied into the buffer
		//this should *not* cause the checkbox to flip to having a unique value
		//However, any subsequent imports from boost indicate intentionality, and if minht is specified, that should be respected
		if (_minht.importFromBoost()) {
			_sameMinHt.setState(false);
		}
		_mindist.importFromBoost();
		_idAlgo.importFromBoost();
		_segAlgo.importFromBoost();
	}
	void TaoParameter::updateUnits() {
		_minht.updateUnits();
		_mindist.updateUnits();
	}
	bool TaoParameter::prepareForRun() {

		if (_runPrepared) {
			return true;
		}

		if (!RunParameters::singleton().doTaos()) {
			_runPrepared = true;
			return true;
		}

		LapisLogger& log = LapisLogger::getLogger();

		if (!_sameMinHt.currentState()) {
			if (std::isnan(_minht.getValueLogErrors())) {
				return false;
			}
			if (_minht.getValueLogErrors() < 0) {
				log.logMessage("Minimum tree height is negative. Is this intentional?");
			}
		}

		if (std::isnan(_mindist.getValueLogErrors())) {
			return false;
		}
		if (_mindist.getValueLogErrors() < 0) {
			log.logMessage("Minimum TAO Distance cannot be negative");
			return false;
		}

		RunParameters& rp = RunParameters::singleton();
		Unit outXYUnits = rp.userCrs().getXYUnits();
		Unit userXYUnits = rp.outUnits();

		switch (_idAlgo.currentSelection()) {
		case IdAlgo::HIGHPOINT:
			_idAlgorithm = std::make_unique<HighPoints>(minTaoHt(), convertUnits(minTaoDist(), userXYUnits, outXYUnits));
			break;
		default:
			log.logMessage("Invalid TAO ID algorithm");
			return false;
		}

		switch (_segAlgo.currentSelection()) {
		case SegAlgo::WATERSHED:
			_segmentAlgorithm = std::make_unique<WatershedSegment>(minTaoHt(),rp.maxHt(),rp.binSize());
			break;
		default:
			log.logMessage("Invalid TAO Segment algorithm");
			return false;
		}

		_runPrepared = true;
		return true;
	}
	void TaoParameter::cleanAfterRun() {
		_idAlgorithm.reset();
		_segmentAlgorithm.reset();
		_runPrepared = false;
	}
	coord_t TaoParameter::minTaoHt() const
	{
		return _sameMinHt.currentState() ? RunParameters::singleton().canopyCutoff() : _minht.getValueLogErrors();
	}
	coord_t TaoParameter::minTaoDist() const
	{
		return _mindist.getValueLogErrors();
	}
	TaoIdAlgorithm* TaoParameter::taoIdAlgo()
	{
		prepareForRun();
		return _idAlgorithm.get();
	}
	TaoSegmentAlgorithm* TaoParameter::taoSegAlgo()
	{
		prepareForRun();
		return _segmentAlgorithm.get();
	}
	int TaoParameter::IdAlgoDecider::operator()(const std::string& s) const
	{
		const static std::regex highpointregex{ ".*high.*",std::regex::icase };
		if (std::regex_match(s, highpointregex)) {
			return IdAlgo::HIGHPOINT;
		}
		return IdAlgo::OTHER;
	}
	std::string TaoParameter::IdAlgoDecider::operator()(int i) const
	{
		if (i == IdAlgo::HIGHPOINT) {
			return "highpoint";
		}
		return "other";
	}
	int TaoParameter::SegAlgoDecider::operator()(const std::string& s) const
	{
		const static std::regex watershedregex{ ".*water.*",std::regex::icase };
		if (std::regex_match(s, watershedregex)) {
			return SegAlgo::WATERSHED;
		}
		return SegAlgo::OTHER;
	}
	std::string TaoParameter::SegAlgoDecider::operator()(int i) const
	{
		if (i == SegAlgo::WATERSHED) {
			return "watershed";
		}
		return "other";
	}
}