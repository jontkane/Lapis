#include"param_pch.hpp"
#include"FineIntParameter.hpp"
#include"RunParameters.hpp"

namespace lapis {

	size_t FineIntParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new FineIntParameter());
	void FineIntParameter::reset()
	{
		*this = FineIntParameter();
	}

	FineIntParameter::FineIntParameter() {
		_sameCellsize.setState(true);
		_sameCutoff.setState(true);

		_useCutoff.addHelpText("Depending on your application, you may want the mean intensity to include all returns, or just returns from trees.\n\b"
			"As with point metrics, the canopy is defined as being returns that are at least a certain height above the ground.");
	}
	void FineIntParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_cellsize.addToCmd(visible, hidden);
		_useCutoff.addToCmd(visible, hidden);
		_cutoff.addToCmd(visible, hidden);
	}
	std::ostream& FineIntParameter::printToIni(std::ostream& o) {
		if (!_sameCellsize.currentState()) {
			_cellsize.printToIni(o);
		}
		_useCutoff.printToIni(o);
		if (_useCutoff.currentState()) {
			if (!_sameCutoff.currentState()) {
				_cutoff.printToIni(o);
			}
		}
		return o;
	}
	ParamCategory FineIntParameter::getCategory() const {
		return ParamCategory::process;
	}
	void FineIntParameter::renderGui() {
		ImGui::Text("Cellsize:");
		_sameCellsize.renderGui();
		ImGui::SameLine();
		if (_sameCellsize.currentState()) {
			ImGui::BeginDisabled();
		}
		_cellsize.renderGui();
		if (_sameCellsize.currentState()) {
			ImGui::EndDisabled();
		}

		ImGui::Dummy(ImVec2(0, 10));
		ImGui::Text("Height Cutoff:");
		_useCutoff.renderGui();
		if (_useCutoff.currentState()) {
			_sameCutoff.renderGui();
			ImGui::SameLine();
			if (_sameCutoff.currentState()) {
				ImGui::BeginDisabled();
			}
			_cutoff.renderGui();
			if (_sameCutoff.currentState()) {
				ImGui::EndDisabled();
			}
		}
	}
	void FineIntParameter::importFromBoost() {

		_useCutoff.importFromBoost();
		if (_cutoff.importFromBoost() && _useCutoff.currentState()) {
			if (!_initialSetup) {
				_sameCutoff.setState(false);
			}
		}
		if (_cellsize.importFromBoost()) {
			if (!_initialSetup) {
				_sameCellsize.setState(false);
			}
		}
		_initialSetup = false;
	}
	void FineIntParameter::updateUnits() {
		_cellsize.updateUnits();
		_cutoff.updateUnits();
	}
	bool FineIntParameter::prepareForRun() {
		if (_runPrepared) {
			return true;
		}
		LapisLogger& log = LapisLogger::getLogger();
		RunParameters& rp = RunParameters::singleton();

		if (!rp.doFineInt()) {
			_runPrepared = true;
			return true;
		}

		if (_useCutoff.currentState()) {
			if (std::isnan(_cutoff.getValueLogErrors())) {
				return false;
			}
			if (_cutoff.getValueLogErrors() < 0) {
				log.logMessage("Fine Intensity cutoff is negative. Is this intentional?");
			}
		}

		coord_t cellsize = 1;
		if (_sameCellsize.currentState()) {
			const Alignment& a = *rp.csmAlign();
			cellsize = a.xres();
		}
		else {
			cellsize = _cellsize.getValueLogErrors();
			if (std::isnan(cellsize)) {
				return false;
			}
			if (cellsize <= 0) {
				log.logMessage("Fine Intensity Cellsize is Negative");
				_runPrepared = true;
				return false;
			}
		}

		const Alignment& a = *rp.metricAlign();
		_fineIntAlign = std::make_unique<Alignment>((Extent)a, a.xOrigin(), a.yOrigin(), cellsize, cellsize);

		_runPrepared = true;
		return true;
	}
	void FineIntParameter::cleanAfterRun() {
		_fineIntAlign.reset();
		_runPrepared = false;
	}
	std::shared_ptr<Alignment> FineIntParameter::fineIntAlign()
	{
		prepareForRun();
		return _fineIntAlign;
	}
	coord_t FineIntParameter::fineIntCutoff() const
	{
		if (!_useCutoff.currentState()) {
			return std::numeric_limits<coord_t>::lowest();
		}
		return _sameCutoff.currentState() ? RunParameters::singleton().canopyCutoff() : _cutoff.getValueLogErrors();
	}
}