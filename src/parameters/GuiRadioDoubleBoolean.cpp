#include"param_pch.hpp"
#include"GuiRadioDoubleBoolean.hpp"

namespace lapis {
	RadioDoubleBoolean::RadioDoubleBoolean(const std::string& firstCmdName, const std::string& secondCmdName)
		: _firstCmdName(firstCmdName), _secondCmdName(secondCmdName), GuiCmdElement("", "")
	{
	}
	void RadioDoubleBoolean::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
	{
		addToCmdBase<bool>(visible, hidden, &_firstBoost);
		addToCmdBase<bool>(visible, hidden, &_secondBoost);
	}
	std::ostream& RadioDoubleBoolean::printToIni(std::ostream& o) const
	{
		if (currentState(FIRST)) {
			o << _firstCmdName << "=\n";
		}
		if (currentState(SECOND)) {
			o << _secondCmdName << "=\n";
		}
		return o;
	}
	bool RadioDoubleBoolean::renderGui()
	{
		bool changed = false;

		bool init = false;
		for (auto& v : _buttons) {
			if (!init) {
				init = true;
			}
			else {
				ImGui::SameLine();
			}
			std::string label = v.second.displayString + "##" + _firstCmdName + _secondCmdName;
			changed = ImGui::RadioButton(label.c_str(), &_radio, v.first) || changed;
		}
		displayHelp();
		return changed;
	}
	bool RadioDoubleBoolean::importFromBoost()
	{
		for (auto& v : _buttons) {
			if (v.second.first == _firstBoost && v.second.second == _secondBoost) {
				_radio = v.first;
			}
		}
		return true;
	}
	bool RadioDoubleBoolean::currentState(int which) const
	{
		if (which == FIRST) {
			return _buttons.at(_radio).first;
		}
		else {
			return _buttons.at(_radio).second;
		}
	}
	void RadioDoubleBoolean::addCombo(bool firstState, bool secondState, int ordering, const std::string& displayString)
	{
		_buttons[ordering] = { firstState, secondState, displayString };
		if (!_defaultSet) {
			_radio = ordering;
			_defaultSet = true;
		}
	}
}