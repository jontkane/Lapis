#pragma once
#ifndef LP_GUIRADIODOUBLEBOOLEAN_H
#define LP_GUIRADIODOUBLEBOOLEAN_H

#include"GuiCmdElement.hpp"

namespace lapis {
	//a slightly strange class where a pair of booleans on the command line is represented by a single radio in the gui, expressing the combinations of those booleans
	class RadioDoubleBoolean : public GuiCmdElement {
	public:
		RadioDoubleBoolean(const std::string& firstCmdName, const std::string& secondCmdName);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;
		bool importFromBoost() override;

		bool currentState(int which) const;

		//the first call to this function sets the default, but the ordering in the gui is decided by the passed int
		void addCombo(bool firstState, bool secondState, int ordering, const std::string& displayString);

		static constexpr int FIRST = 0;
		static constexpr int SECOND = 1;

	private:
		std::string _firstCmdName;
		std::string _secondCmdName;

		struct SingleButton {
			bool first;
			bool second;
			std::string displayString;
		};
		std::map<int, SingleButton> _buttons;

		int _radio = -1;
		bool _firstBoost = false;
		bool _secondBoost = false;

		bool _defaultSet = false;
	};
}

#endif