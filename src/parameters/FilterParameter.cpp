#include"param_pch.hpp"
#include"FilterParameter.hpp"
#include"RunParameters.hpp"

namespace lapis {

	size_t FilterParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new FilterParameter());
	void FilterParameter::reset()
	{
		*this = FilterParameter();
	}

	FilterParameter::FilterParameter() {
		for (size_t i = 0; i < 256; ++i) {
			_class.setState(i, true);
		}
		_class.setState(7, false);
		_class.setState(9, false);
		_class.setState(18, false);

		_minht.addHelpText("Ocassionally, points in a laz file will be so far above or below the ground as to be obvious errors.\n\n"
			"You can specify those cutoffs here.");
		_class.addHelpText("Your laz files may have classification information indicating a best guess at what the lidar pulse struck.\n\n"
			"You can choose to exclude certain classifications from the analysis.");
		_filterWithheld.addHelpText("Some points in laz files are marked as withheld, indicating that they should not be used.\n\n"
			"For the vast majority of applications, they should be excluded, but you can choose to include them with this checkbox.");
	}
	void FilterParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_minht.addToCmd(visible, hidden);
		_maxht.addToCmd(visible, hidden);
		_class.addToCmd(visible, hidden);
		_filterWithheld.addToCmd(visible, hidden);
	}
	std::ostream& FilterParameter::printToIni(std::ostream& o) {
		_minht.printToIni(o);
		_maxht.printToIni(o);
		_filterWithheld.printToIni(o);
		_class.printToIni(o);
		return o;
	}
	ParamCategory FilterParameter::getCategory() const {
		return ParamCategory::process;
	}
	void FilterParameter::renderGui() {
		_filterWithheld.renderGui();
		_minht.renderGui();
		_maxht.renderGui();

		ImGui::Text("Use Classes:");
		ImGui::SameLine();
		_class.renderGui();
	}
	void FilterParameter::updateUnits() {
		_minht.updateUnits();
		_maxht.updateUnits();
	}
	void FilterParameter::importFromBoost() {
		_minht.importFromBoost();
		_maxht.importFromBoost();
		_filterWithheld.importFromBoost();
		_class.importFromBoost();
	}
	bool FilterParameter::prepareForRun() {
		if (_runPrepared) {
			return true;
		}

		if (std::isnan(_minht.getValueLogErrors()) || std::isnan(_maxht.getValueLogErrors())) {
			return false;
		}

		if (_minht.getValueLogErrors() > _maxht.getValueLogErrors()) {
			LapisLogger& log = LapisLogger::getLogger();
			log.logMessage("Low outlier must be less than high outlier");
			return false;
		}
		if (_filterWithheld.currentState()) {
			_filters.emplace_back(new LasFilterWithheld());
		}
		_filters.push_back(_class.getFilter());
		_runPrepared = true;
		return true;
	}
	void FilterParameter::cleanAfterRun() {
		_filters.clear();
		_filters.shrink_to_fit();
		_runPrepared = false;
	}
	const std::vector<std::shared_ptr<LasFilter>>& FilterParameter::filters()
	{
		prepareForRun();
		return _filters;
	}
	coord_t FilterParameter::minht() const
	{
		return _minht.getValueLogErrors();
	}
	coord_t FilterParameter::maxht() const
	{
		return _maxht.getValueLogErrors();
	}
}