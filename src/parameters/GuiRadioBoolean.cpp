#include"param_pch.hpp"
#include"GuiRadioBoolean.hpp"

namespace lapis {

	RadioBoolean::RadioBoolean(const std::string& cmdName, const std::string& trueDisplayString, const std::string& falseDisplayString)
		: GuiCmdElement("", cmdName), _trueDisplayString(trueDisplayString + "##" + _cmdName), _falseDisplayString(falseDisplayString + "##" + _cmdName)
	{
	}
	RadioBoolean::RadioBoolean(const std::string& cmdName, const std::string& trueDisplayString, const std::string& falseDisplayString, const std::string& cmdDescription)
		: GuiCmdElement("", cmdName, cmdDescription), _trueDisplayString(trueDisplayString), _falseDisplayString(falseDisplayString)
	{
	}
	void RadioBoolean::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
	{
		addToCmdBase(visible, hidden, &_boostState);
	}
	std::ostream& RadioBoolean::printToIni(std::ostream& o) const
	{
		if (_radio == TRUE_RADIO) {
			o << _cmdName << "=\n";
		}

		return o;
	}
	bool RadioBoolean::renderGui()
	{
		bool changed = false;
		if (_trueFirst) {
			changed = ImGui::RadioButton(_trueDisplayString.c_str(), &_radio, TRUE_RADIO) || changed;
			ImGui::SameLine();
			changed = ImGui::RadioButton(_falseDisplayString.c_str(), &_radio, FALSE_RADIO) || changed;
		}
		else {
			changed = ImGui::RadioButton(_falseDisplayString.c_str(), &_radio, FALSE_RADIO) || changed;
			ImGui::SameLine();
			changed = ImGui::RadioButton(_trueDisplayString.c_str(), &_radio, TRUE_RADIO) || changed;
		}
		displayHelp();
		return changed;
	}
	bool RadioBoolean::importFromBoost()
	{
		if (_boostState) {
			_radio = TRUE_RADIO;
		}
		else {
			_radio = FALSE_RADIO;
		}
		return true;
	}
	bool RadioBoolean::currentState() const
	{
		return _radio == TRUE_RADIO;
	}
	void RadioBoolean::setState(bool b)
	{
		if (b) {
			_radio = TRUE_RADIO;
		}
		else {
			_radio = FALSE_RADIO;
		}
	}
	void RadioBoolean::displayFalseFirst()
	{
		_trueFirst = false;
	}
}