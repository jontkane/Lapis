#include"param_pch.hpp"
#include"GuiCheckBox.hpp"

namespace lapis {
	CheckBox::CheckBox(const std::string& guiDesc, const std::string& cmdName)
		: _state(false), _boostState(false), GuiCmdElement(guiDesc, cmdName)
	{
	}
	CheckBox::CheckBox(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription)
		: _state(false), _boostState(false), GuiCmdElement(guiDesc, cmdName, cmdDescription)
	{
	}
	void CheckBox::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
	{
		addToCmdBase<bool>(visible, hidden, &_boostState);
	}
	std::ostream& CheckBox::printToIni(std::ostream& o) const
	{
		if (_state) {
			o << _cmdName << "=\n";
		}
		return o;
	}
	bool CheckBox::renderGui()
	{
		std::string label = _guiDesc + "##" + _cmdName;
		bool changed = ImGui::Checkbox(label.c_str(), &_state);
		displayHelp();
		return changed;
	}
	bool CheckBox::importFromBoost()
	{
		_state = _boostState;
		return true;
	}
	bool CheckBox::currentState() const
	{
		return _state;
	}
	void CheckBox::setState(bool b)
	{
		_state = b;
	}
}