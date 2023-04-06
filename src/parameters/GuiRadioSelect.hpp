#pragma once
#ifndef LP_GUIRADIOSELECT_H
#define LP_GUIRADIOSELECT_H

#include"GuiCmdElement.hpp"

namespace lapis {
	
	//This class represents a method of selection between multiple options: in the gui, via a radio button, and in the command line, via regex matching with using input
	//DECIDER is a functor class with two overloads of operator():
	//one takes a string as provided by the user on the command line, and converts that to an int representing which radio option is selected
	//the other takes an int representing a radio button, and returns a string suitable which would produce that int in the other function
	template<class DECIDER, class VALUE>
	class RadioSelect : public GuiCmdElement {
	public:
		struct SingleButton {
			std::string display;
			VALUE value = VALUE();
		};

		RadioSelect(const std::string& guiDesc, const std::string& cmdName);
		RadioSelect(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;

		bool importFromBoost() override;

		const VALUE& currentSelection() const;

		void setSelection(int whichButton);

		//whichButton should be non-negative. The first call of this function also determines the default value, but otherwise the order of the
		//calls doesn't matter--the buttons are sorted by whichButton, not by the order this function is called
		void addOption(const std::string& display, int whichButton, const VALUE& value);

		void setSingleLine();

	private:
		int _radio = -1;
		std::map<int, SingleButton> _buttons;
		std::string _boostString;

		DECIDER _decider;

		bool _singleLine = false;
	};

	template<class DECIDER, class VALUE>
	inline RadioSelect<DECIDER, VALUE>::RadioSelect(const std::string& guiDesc, const std::string& cmdName)
		: GuiCmdElement(guiDesc, cmdName)
	{
	}
	template<class DECIDER, class VALUE>
	inline RadioSelect<DECIDER, VALUE>::RadioSelect(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription)
		: GuiCmdElement(guiDesc, cmdName, cmdDescription)
	{
	}
	template<class DECIDER, class VALUE>
	inline void RadioSelect<DECIDER, VALUE>::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
	{
		addToCmdBase(visible, hidden, &_boostString);
	}
	template<class DECIDER, class VALUE>
	inline std::ostream& RadioSelect<DECIDER, VALUE>::printToIni(std::ostream& o) const
	{
		o << _cmdName << "=" << _decider(_radio) << "\n";
		return o;
	}
	template<class DECIDER, class VALUE>
	inline bool RadioSelect<DECIDER, VALUE>::renderGui()
	{
		bool changed = false;
		if (_guiDesc.size()) {
			ImGui::Text(_guiDesc.c_str());
		}

		if (!_singleLine) {
			displayHelp();
		}

		for (auto& v : _buttons) {
			if (_singleLine) {
				ImGui::SameLine();
			}
			changed = ImGui::RadioButton(v.second.display.c_str(), &_radio, v.first) || changed;
		}
		if (_singleLine) {
			displayHelp();
		}
		return changed;
	}
	template<class DECIDER, class VALUE>
	inline bool RadioSelect<DECIDER, VALUE>::importFromBoost()
	{
		if (!_boostString.size()) {
			return false;
		}

		int candidate = _decider(_boostString);
		if (_buttons.contains(candidate)) {
			_radio = candidate;
		}
		else {
			LapisLogger::getLogger().logMessage(_boostString + " is not a valid option for " + _cmdName);
		}

		_boostString.clear();
		return true;
	}
	template<class DECIDER, class VALUE>
	inline const VALUE& RadioSelect<DECIDER, VALUE>::currentSelection() const
	{
		return _buttons.at(_radio).value;
	}
	template<class DECIDER, class VALUE>
	inline void RadioSelect<DECIDER, VALUE>::setSelection(int whichButton)
	{
		_radio = whichButton;
	}
	template<class DECIDER, class VALUE>
	inline void RadioSelect<DECIDER, VALUE>::addOption(const std::string& display, int whichButton, const VALUE& value)
	{
		if (_radio < 0) {
			_radio = whichButton;
		}
		_buttons[whichButton] = { display, value };
	}
	template<class DECIDER, class VALUE>
	inline void RadioSelect<DECIDER, VALUE>::setSingleLine()
	{
		_singleLine = true;
	}


	namespace UnitRadio {
		enum {
			UNKNOWN = 0,
			METERS = 1,
			INTFEET = 2,
			USFEET = 3
		};
	}
	class UnitDecider {
	public:
		int operator()(const std::string& s) const {
			{
				static const std::regex meterpattern{ ".*m.*",std::regex::icase };
				static const std::regex usfootpattern{ ".*us.*",std::regex::icase };
				static const std::regex unspecifiedpattern{ ".*unsp.*",std::regex::icase };
				static const std::regex intfootpattern{ ".*f.*",std::regex::icase };

				if (std::regex_match(s, unspecifiedpattern)) {
					return UnitRadio::UNKNOWN;
				}
				if (std::regex_match(s, meterpattern)) {
					return UnitRadio::METERS;
				}
				if (std::regex_match(s, usfootpattern)) {
					return UnitRadio::USFEET;
				}
				if (std::regex_match(s, intfootpattern)) {
					return UnitRadio::INTFEET;
				}
				return UnitRadio::UNKNOWN;
			}
		}
		std::string operator()(int i) const
		{
			if (i == UnitRadio::METERS) {
				return "meters";
			}
			if (i == UnitRadio::INTFEET) {
				return "intfeet";
			}
			if (i == UnitRadio::USFEET) {
				return "usfeet";
			}
			return "unspecified";
		}
	};

	using UnitRadioSelect = RadioSelect<UnitDecider, LinearUnit>;
}

#endif