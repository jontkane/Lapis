#pragma once
#ifndef LP_GUICHECKBOX_H
#define LP_GUICHECKBOX_H

#include"GuiCmdElement.hpp"

namespace lapis {
	//a parameter which is a simple bool, which defaults to false. The presence of a command line argument switches it to true.
	class CheckBox : public GuiCmdElement {
	public:
		CheckBox(const std::string& guiDesc, const std::string& cmdName);
		CheckBox(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;
		bool importFromBoost() override;

		bool currentState() const;
		void setState(bool b);

	private:
		bool _state = false;
		bool _boostState = false;
	};
}

#endif