#include"param_pch.hpp"
#include"TaoParameter.hpp"
#include"ParameterGetter.hpp"
#include"..\algorithms\AllTaoIdAlgorithms.hpp"
#include"..\algorithms\AllTaoSegmentAlgorithms.hpp"

namespace lapis {

	LAPIS_PARAMETER_REGISTER_DEFINE(TaoParameter);
	void TaoParameter::reset()
	{
		*this = TaoParameter();
	}

	TaoParameter::TaoParameter() {
		//static_assert(false, "Need to modify this for the tao refactor and new algorithms");
		_idAlgo.addOption("High Points", IdAlgo::HIGHPOINT, IdAlgo::HIGHPOINT);
		_idAlgo.setSingleLine();
		_idAlgo.addHelpText("The algorithm for identifying trees. Only the 'high points' algorithm is supported currently.\n\n"
			"High Points: This is a CSM-based algorithm. A CSM pixel is considered a candidate for being the stem of a tree if it's higher than all 8 of its neighbors.\n"
			"It is good for trees such as conifers with well-defined tops.");

		_sameMinHt.setState(true);

		_mindist.addHelpText("If two TAOs are very close to each other, it may represent an error rather than two separate trees. "
			"If this is set to a value greater than 0, then if two TAOs are too close, the shorter one will be removed.");

		_doWatershed.addHelpText("This algorithm marks the boundary between TAOs as the location where the canopy has a 'valley'. "
			"It has the advantage that all canopy pixels are assigned to exactly one TAO, and is fast to run. "
			"It has the disadvantage that the segments produced often do not resemble the shape of trees.");

		_vectorizeWatershed.addHelpText("If this box is checked, vectorized polygons will be produced corresponding to the boundary of each TAO.\n"
			"This option is fairly slow and may increase the memory requirements of the run.");

		_doMcgaughey.addHelpText("This algorithm is found in the FUSION program, created by Bob McGaughey. "
			"It produces only polygons, no rasters. It works by casting many rays out from the TAO. "
			"When the ray encounters one of several conditions, it stops and that location is recorded as a vertex of the polygon. ");
		_mcgSlopechangeMultiplier.addHelpText("One of the conditions for a boundary of a TAO is when the slope of the CSM suddenly increased (i.e., there is a sharp drop-off). "
			"This variable controls how sharp that drop-off needs to be before it causes a boundary to be found. A higher value requires a sharper drop-off.");
		_mcgHeightCutoffMultiplier.addHelpText("One of the conditions for a boundary of a TAO is when the CSM drops below a certain percentage of the height of the TAO. "
			"This variable controls that percentage. For example, by default, areas lower than 2/3 the height of the TAO cannot be part of the segment.");
		_mcgMaxDistMultiplier.addHelpText("The boundary of the segment cannot be more than a certain distance from the TAO, measured as a percentage of the height of the TAO. "
            "This variable controls that percentage. For example, by default, the segment will never have a radius larger than 3/4 the height of the TAO.");
		_mcgNvertices.addHelpText("The number of vertices for the output polygons. "
			"A higher number will produce a smoother polygon, but will take longer to compute.");
		_mcgSmoothType.addHelpText("The type of smoothing to apply to the polygon.\n\n"
			"Fusion: The original algorithm from FUSION. This algorithm has a number of complexities, but to simplify, it smooths 'spikes' in the polygon by setting their distance from the TAO to be "
			"equal to the average distance of the two neighboring vertices.\n\n"
			"Simple: This algorithm simply sets the distance of each vertex to be the average of its own distance, and the distances of the two neighboring vertices.\n\n"
			"None: No smoothing is performed.");
        _mcgSmoothType.addOption("Fusion", (int)McGaugheySmoothType::fusion, McGaugheySmoothType::fusion);
        _mcgSmoothType.addOption("Simple", (int)McGaugheySmoothType::simple, McGaugheySmoothType::simple);
        _mcgSmoothType.addOption("None", (int)McGaugheySmoothType::none, McGaugheySmoothType::none);
        _mcgSmoothType.setSingleLine();

	}
	void TaoParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_minht.addToCmd(visible, hidden);
		_mindist.addToCmd(visible, hidden);
		_idAlgo.addToCmd(visible, hidden);

		_doWatershed.addToCmd(visible, hidden);
		_vectorizeWatershed.addToCmd(visible, hidden);

        _doMcgaughey.addToCmd(visible, hidden);
        _mcgNvertices.addToCmd(visible, hidden);
        _mcgSlopechangeMultiplier.addToCmd(visible, hidden);
        _mcgHeightCutoffMultiplier.addToCmd(visible, hidden);
        _mcgMaxDistMultiplier.addToCmd(visible, hidden);
        _mcgSmoothType.addToCmd(visible, hidden);
	}
	std::ostream& TaoParameter::printToIni(std::ostream& o) {
		if (!_sameMinHt.currentState()) {
			_minht.printToIni(o);
		}
		_mindist.printToIni(o);

		_idAlgo.printToIni(o);

        _doWatershed.printToIni(o);
        _vectorizeWatershed.printToIni(o);

        _doMcgaughey.printToIni(o);
        _mcgNvertices.printToIni(o);
        _mcgSlopechangeMultiplier.printToIni(o);
        _mcgHeightCutoffMultiplier.printToIni(o);
        _mcgMaxDistMultiplier.printToIni(o);
        _mcgSmoothType.printToIni(o);

		return o;
	}
	ParamCategory TaoParameter::getCategory() const {
		return ParamCategory::process;
	}
	void TaoParameter::renderGui() {
		if (!parameterManager().doCsm()) {
			ImGui::Text("Tree identification requires a CSM");
			return;
		}

		_title.renderGui();

		_idAlgo.renderGui();
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


		ImGui::Text("Segmentation Aglorithms: ");
		ImGui::SameLine();
		_doWatershed.renderGui();
		ImGui::SameLine();
        _doMcgaughey.renderGui();

		ImGui::BeginTabBar("Segmentation Algorithms");

		if (_doWatershed.currentState()) {
			if (ImGui::BeginTabItem("Watershed")) {
				_vectorizeWatershed.renderGui();
				ImGui::EndTabItem();
			}
		}
		if (_doMcgaughey.currentState()) {
			if (ImGui::BeginTabItem("McGaughey")) {
				_mcgNvertices.renderGui();
				_mcgSlopechangeMultiplier.renderGui();
				_mcgHeightCutoffMultiplier.renderGui();
				_mcgMaxDistMultiplier.renderGui();
				_mcgSmoothType.renderGui();
				ImGui::EndTabItem();
			}
        }

		ImGui::EndTabBar();
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

        _doWatershed.importFromBoost();
        _vectorizeWatershed.importFromBoost();

        _doMcgaughey.importFromBoost();
        _mcgNvertices.importFromBoost();
        _mcgSlopechangeMultiplier.importFromBoost();
        _mcgHeightCutoffMultiplier.importFromBoost();
        _mcgMaxDistMultiplier.importFromBoost();
        _mcgSmoothType.importFromBoost();

		//if doTaos is true, then we should default to having at least one segmentation algorithm on
		//watershed seems like a reasonable default
        ParameterManager& pm = parameterManager();
		if (pm.doTaos()) {
			int segAlgoCount = 0;
            segAlgoCount += _doWatershed.currentState() ? 1 : 0;
            segAlgoCount += _doMcgaughey.currentState() ? 1 : 0;

			if (segAlgoCount == 0) {
				_doWatershed.setState(true);
			}
		}
	}
	void TaoParameter::updateUnits() {
		_minht.updateUnits();
		_mindist.updateUnits();
	}
	bool TaoParameter::prepareForRun() {

		if (_runPrepared) {
			return true;
		}

		if (!parameterManager().doTaos()) {
			_runPrepared = true;
			return true;
		}

		LapisLogger& log = LapisLogger::getLogger();

		if (!_sameMinHt.currentState()) {
			if (std::isnan(_minht.getValueLogErrors())) {
				return false;
			}
			if (_minht.getValueLogErrors() < 0) {
				log.logWarning("Minimum tree height is negative. Is this intentional?");
			}
		}

		if (std::isnan(_mindist.getValueLogErrors())) {
			return false;
		}
		if (_mindist.getValueLogErrors() < 0) {
			log.logError("Minimum TAO Distance cannot be negative");
			return false;
		}

		ParameterManager& pm = parameterManager();
		//any scenario where this wouldn't have a value should be caught earlier in the code
		LinearUnit outXYUnits = pm.outputCrs().getXYLinearUnits().value();
		LinearUnit userXYUnits = pm.outUnits();
		switch (_idAlgo.currentSelection()) {
		case IdAlgo::HIGHPOINT:
			_idAlgorithm = std::make_unique<HighPoints>(minTaoHt(), userXYUnits.convertOneFromThis(minTaoDist(), outXYUnits));
			break;
		default:
			log.logError("Invalid TAO ID algorithm");
			return false;
		}

		if (_doWatershed.currentState()) {
			_segmentAlgorithms.emplace_back(new WatershedSegment(minTaoHt(), pm.maxHt(), pm.binSize(), _vectorizeWatershed.currentState()));
		}
		if (_doMcgaughey.currentState()) {
			if (_mcgNvertices.getValueLogErrors() < 3) {
				log.logError("Number of vertices for McGaughey algorithm must be at least 3");
				return false;
            }
			if (_mcgSlopechangeMultiplier.getValueLogErrors() <= 0) {
				log.logError("Slope change multiplier for McGaughey algorithm must be greater than 0");
				return false;
			}
			if (_mcgHeightCutoffMultiplier.getValueLogErrors() <= 0) {
				log.logError("Height cutoff multiplier for McGaughey algorithm must be greater than 0");
				return false;
            }
			if (_mcgHeightCutoffMultiplier.getValueLogErrors() >= 1) {
				log.logError("Height cutoff multiplier for McGaughey algorithm must be less than 1");
				return false;
            }
			if (_mcgMaxDistMultiplier.getValueLogErrors() <= 0) {
				log.logError("Maximum distance multiplier for McGaughey algorithm must be greater than 0");
				return false;
			}
			_segmentAlgorithms.emplace_back(new McGaugheySegment(
				(int)_mcgNvertices.getValueLogErrors(),
				_mcgSlopechangeMultiplier.getValueLogErrors(),
				_mcgHeightCutoffMultiplier.getValueLogErrors(),
				_mcgMaxDistMultiplier.getValueLogErrors(),
				_mcgSmoothType.currentSelection()));

		}

		_runPrepared = true;
		return true;
	}
	void TaoParameter::cleanAfterRun() {
		_idAlgorithm.reset();
		_segmentAlgorithms.clear();
		_runPrepared = false;
	}
	coord_t TaoParameter::minTaoHt() const
	{
		return _sameMinHt.currentState() ? parameterManager().canopyCutoff() : _minht.getValueLogErrors();
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
	const std::vector<std::unique_ptr<TaoSegmentAlgorithm>>& TaoParameter::taoSegAlgos()
	{
		prepareForRun();
		return _segmentAlgorithms;
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

	int TaoParameter::McGaugheySmoothDecider::operator()(const std::string& s) const
	{
		const static std::regex fusionregex{ ".*fusion.*",std::regex::icase };
		const static std::regex simpleregex{ ".*simple.*",std::regex::icase };
		if (std::regex_match(s, fusionregex)) {
			return (int)McGaugheySmoothType::fusion;
		}
		else if (std::regex_match(s, simpleregex)) {
			return (int)McGaugheySmoothType::simple;
		}
		return (int)McGaugheySmoothType::none;
    }
    std::string TaoParameter::McGaugheySmoothDecider::operator()(int i) const
	{
		switch ((McGaugheySmoothType)i) {
		case McGaugheySmoothType::fusion:
			return "fusion";
		case McGaugheySmoothType::simple:
			return "simple";
		case McGaugheySmoothType::none:
		default:
			return "none";
		}
    }
}