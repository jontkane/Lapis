#pragma once
#ifndef LP_GUIRADIOBOOLEAN_H
#define LP_GUIRADIOBOOLEAN_H

#include"GuiCmdElement.hpp"

namespace lapis {

	//a parameter which is a simple bool, but which is selected on the gui as two radio buttons
	class RadioBoolean : public GuiCmdElement {
	public:
		RadioBoolean(const std::string& cmdName,
			const std::string& trueDisplayString, const std::string& falseDisplayString);
		RadioBoolean(const std::string& cmdName,
			const std::string& trueDisplayString, const std::string& falseDisplayString,
			const std::string& cmdDescription);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;
		bool importFromBoost() override;

		bool currentState() const;
		void setState(bool b);

		void displayFalseFirst();

	private:
		static constexpr int TRUE_RADIO = 1;
		static constexpr int FALSE_RADIO = 0;
		int _radio = FALSE_RADIO;
		bool _boostState = false;
		std::string _trueDisplayString;
		std::string _falseDisplayString;

		bool _trueFirst = true;
	};
}

#endif