#include"param_pch.hpp"
#include"GuiInvertedCheckbox.hpp"

namespace lapis {
	InvertedCheckBox::InvertedCheckBox(const std::string& guiDesc, const std::string& cmdName)
		: _state(true), _boostState(false), GuiCmdElement(guiDesc, cmdName)
	{
	}
	InvertedCheckBox::InvertedCheckBox(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription)
		: _state(true), _boostState(false), GuiCmdElement(guiDesc, cmdName, cmdDescription)
	{
	}
	void InvertedCheckBox::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
	{
		addToCmdBase<bool>(visible, hidden, &_boostState);
	}
	std::ostream& InvertedCheckBox::printToIni(std::ostream& o) const
	{
		if (!_state) {
			o << _cmdName << "=\n";
		}
		return o;
	}
	bool InvertedCheckBox::renderGui()
	{
		std::string label = _guiDesc + "##" + _cmdName;
		bool changed = ImGui::Checkbox(label.c_str(), &_state);
		displayHelp();
		return changed;
	}
	bool InvertedCheckBox::importFromBoost()
	{
		_state = !_boostState;
		return true;
	}
	bool InvertedCheckBox::currentState() const
	{
		return _state;
	}
	void InvertedCheckBox::setState(bool b)
	{
		_state = b;
	}
}