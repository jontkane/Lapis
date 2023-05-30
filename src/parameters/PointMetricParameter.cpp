#include"param_pch.hpp"
#include"PointMetricParameter.hpp"
#include"RunParameters.hpp"

namespace lapis {

	size_t PointMetricParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new PointMetricParameter());
	void PointMetricParameter::reset()
	{
		*this = PointMetricParameter();
	}

	PointMetricParameter::PointMetricParameter() {
		_advMetrics.displayFalseFirst();

		_whichReturns.addCombo(DONT_SKIP, DONT_SKIP, 2, "Both");
		_whichReturns.addCombo(DO_SKIP, DONT_SKIP, 0, "All Returns");
		_whichReturns.addCombo(DONT_SKIP, DO_SKIP, 1, "First Returns");

		_canopyCutoff.addHelpText("Many point metrics are either calculated only on canopy points, or treat canopy points specially in some other way.\n\n"
			"A canopy point is defined as being above some specific height, which you specify here.");
		_doStrata.addHelpText("Some metrics are calculated on specific height bands; for example, on all returns between 8 and 16 meters above the ground."
			"These numbers specify those bands.");
		_advMetrics.addHelpText("There are a very large number of point metrics that have been proposed over the years.\n\n"
			"However, a much smaller subset suffice for the vast majority of applications.\n\n"
			"This checkbox controls whether Lapis calculates the more limited set, or the full set.");
		_whichReturns.addHelpText("Depending on the application, you may want point metrics to be calculated either on first returns only, or on all returns.\n\n"
			"You can control that with this selection.");
	}
	void PointMetricParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_canopyCutoff.addToCmd(visible, hidden);
		_strata.addToCmd(visible, hidden);
		_advMetrics.addToCmd(visible, hidden);
		_whichReturns.addToCmd(visible, hidden);
		_doStrata.addToCmd(visible, hidden);
	}
	std::ostream& PointMetricParameter::printToIni(std::ostream& o) {
		_canopyCutoff.printToIni(o);
		_strata.printToIni(o);
		_advMetrics.printToIni(o);
		_whichReturns.printToIni(o);
		_doStrata.printToIni(o);
		return o;
	}
	ParamCategory PointMetricParameter::getCategory() const {
		return ParamCategory::process;
	}
	void PointMetricParameter::renderGui() {
		_title.renderGui();

		ImGui::BeginChild("stratumleft", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 2, ImGui::GetContentRegionAvail().y), false, 0);
		_canopyCutoff.renderGui();
		_advMetrics.renderGui();
		ImGui::Text("Calculate Metrics Using:");
		_whichReturns.renderGui();
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("stratumright", ImVec2(ImGui::GetContentRegionAvail().x - 2, ImGui::GetContentRegionAvail().y), true, 0);
		_doStrata.renderGui();
		if (_doStrata.currentState()) {
			_strata.renderGui();
		}
		ImGui::EndChild();
	}
	void PointMetricParameter::updateUnits() {
		_canopyCutoff.updateUnits();
		_strata.updateUnits();
	}
	void PointMetricParameter::importFromBoost() {

		_canopyCutoff.importFromBoost();
		_strata.importFromBoost();
		_advMetrics.importFromBoost();
		_whichReturns.importFromBoost();
		_doStrata.importFromBoost();
	}
	bool PointMetricParameter::prepareForRun() {

		if (_runPrepared) {
			return true;
		}

		RunParameters& rp = RunParameters::singleton();

		LapisLogger& log = LapisLogger::getLogger();
		if (std::isnan(_canopyCutoff.getValueLogErrors())) {
			return false;
		}
		if (_canopyCutoff.getValueLogErrors() < 0) {
			log.logWarning("Canopy cutoff is negative. Is this intentional?");
		}

		if (!rp.doPointMetrics()) {
			_runPrepared = true;
			return true;
		}

		if (_doStrata.currentState()) {
			if (_strata.cachedValues().size()) {
				auto to_string_with_precision = [](coord_t v)->std::string {
					//all this nonsense should get a non-scientific notation representation with at most two places after the decimal point, but trailing 0s removed
					std::ostringstream out;
					out.precision(2);
					out << std::fixed;
					out << v;
					std::string s = out.str();
					s.erase(s.find_last_not_of('0') + 1, std::string::npos);
					s.erase(s.find_last_not_of('.') + 1, std::string::npos);
					return s;
				};
				const std::vector<coord_t>& strata = _strata.cachedValues();
				_strataNames.clear();
				_strataNames.push_back("LessThan" + to_string_with_precision(strata[0]) + rp.unitPlural());
				for (size_t i = 1; i < strata.size(); ++i) {
					_strataNames.push_back(to_string_with_precision(strata[i - 1]) + "To" + to_string_with_precision(strata[i]) + rp.unitPlural());
				}
				_strataNames.push_back("GreaterThan" + to_string_with_precision(strata[strata.size() - 1]) + rp.unitPlural());
			}
		}

		_runPrepared = true;
		return true;
	}
	void PointMetricParameter::cleanAfterRun() {
		_runPrepared = false;
	}
	bool PointMetricParameter::doAllReturns() const
	{
		return _whichReturns.currentState(ALL_RETURNS) == DONT_SKIP;
	}
	bool PointMetricParameter::doFirstReturns() const
	{
		return _whichReturns.currentState(FIRST_RETURNS) == DONT_SKIP;
	}
	const std::vector<coord_t>& PointMetricParameter::strata() const
	{
		return _strata.cachedValues();
	}
	const std::vector<std::string>& PointMetricParameter::strataNames()
	{
		prepareForRun();
		return _strataNames;
	}
	coord_t PointMetricParameter::canopyCutoff() const
	{
		return _canopyCutoff.getValueLogErrors();
	}
	bool PointMetricParameter::doStratumMetrics() const
	{
		return _doStrata.currentState();
	}
	bool PointMetricParameter::doAdvancedPointMetrics() const
	{
		return _advMetrics.currentState();
	}
}