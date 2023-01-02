#include"param_pch.hpp"
#include"GuiCmdElement.hpp"

namespace lapis {

	GuiCmdElement::GuiCmdElement(const std::string& guiDesc, const std::string& cmdName)
		: _cmdName(cmdName), _guiDesc(guiDesc), _hidden(true), _cmdDesc()
	{
	}
	GuiCmdElement::GuiCmdElement(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription)
		: _guiDesc(guiDesc), _cmdName(cmdName), _cmdDesc(cmdDescription), _hidden(false)
	{
	}
	void GuiCmdElement::addShortCmdAlias(char alias)
	{
		_alias = alias;
	}
	void GuiCmdElement::addHelpText(const std::string& help)
	{
		_helpText = help;
	}
	void GuiCmdElement::displayHelp()
	{
		if (!_helpText.size()) {
			return;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(_helpText.c_str());
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}
}