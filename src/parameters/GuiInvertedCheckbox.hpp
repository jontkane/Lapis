#pragma once
#ifndef LP_GUIINVERTEDCHECKBOX_H
#define LP_GUIINVERTEDCHECKBOX_H

#include"GuiCmdElement.hpp"

namespace lapis {
	//a parameter which is a simple bool, but defaults to true. The presence of a command line argument switches it to false
	class InvertedCheckBox : public GuiCmdElement {
	public:
		InvertedCheckBox(const std::string& guiDesc, const std::string& cmdName);
		InvertedCheckBox(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;
		bool importFromBoost() override;

		bool currentState() const;
		void setState(bool b);

	private:
		bool _state = true;
		bool _boostState = false;
	};
}

#endif