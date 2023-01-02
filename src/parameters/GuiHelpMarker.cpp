#include"param_pch.hpp"
#include"GuiHelpMarker.hpp"

namespace lapis {
	HelpMarker::HelpMarker(const std::string& helpText)
		: GuiCmdElement("","")
	{
		addHelpText(helpText);
	}
	void HelpMarker::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
	{
	}
	std::ostream& HelpMarker::printToIni(std::ostream& o) const
	{
		return o;
	}
	bool HelpMarker::renderGui()
	{
		displayHelp();
		return false;
	}
	bool HelpMarker::importFromBoost()
	{
		return false;
	}
}