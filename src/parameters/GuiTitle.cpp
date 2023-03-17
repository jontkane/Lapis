#include"param_pch.hpp"
#include"GuiCheckBox.hpp"
#include "GuiTitle.hpp"

namespace lapis {
	lapis::Title::Title(const std::string& guiDesc) : GuiCmdElement(guiDesc, "")
	{
	}
	void Title::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
	{
	}
	std::ostream& Title::printToIni(std::ostream& o) const
	{
		return o;
	}
	bool Title::renderGui()
	{
		ImGui::PushFont(getBoldFont());
		ImGui::Text(_guiDesc.c_str());
		ImGui::PopFont();

		if (_helpText.size()) {
			ImGui::SameLine();
			displayHelp();
		}
		return false;
	}
	bool Title::importFromBoost()
	{
		return false;
	}
}