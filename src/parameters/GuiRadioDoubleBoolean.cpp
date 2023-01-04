#include"param_pch.hpp"
#include"GuiRadioDoubleBoolean.hpp"

namespace lapis {
	RadioDoubleBoolean::RadioDoubleBoolean(const std::string& firstCmdName, const std::string& secondCmdName)
		: _firstCmdName(firstCmdName), _secondCmdName(secondCmdName), GuiCmdElement("", "")
	{
	}
	void RadioDoubleBoolean::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
	{
		namespace po = boost::program_options;
		auto addOneToBoost = [&](std::string name, bool* b) {
			if (_alias > 0) {
				name.push_back(',');
				name.push_back(_alias);
			}
			if (_hidden) {
				hidden.add_options()(name.c_str(), po::bool_switch(b), "");
			}
			else {
				visible.add_options()(name.c_str(), po::bool_switch(b), _cmdDesc.c_str());
			}
		};
		addOneToBoost(_firstCmdName, &_firstBoost);
		addOneToBoost(_secondCmdName, &_secondBoost);
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