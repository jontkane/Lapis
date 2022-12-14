#pragma once
#ifndef LP_COMMONGUIELEMENTS_H
#define LP_COMMONGUIELEMENTS_H

#include"app_pch.hpp"
#include"LapisTypedefs.hpp"
#include"LapisLogger.hpp"
#include"LapisUtils.hpp"

namespace lapis {
	
	namespace po = boost::program_options;

	static constexpr size_t LAPIS_NUMERIC_BUFFER_SIZE = 11;

	/*
	* Implementations of this class contain all the functionality needed to handle the conversion between the command line, gui, and internal data object
	* for a single parameter. Anything that cares about the interaction of multiple command line parameters should be handled at a higher level
	*/
	class CmdGuiElement {
	public:

		CmdGuiElement(const std::string& guiDesc, const std::string& cmdName);
		CmdGuiElement(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription);
		virtual ~CmdGuiElement() = default;

		virtual void addToCmd(po::options_description& visible,
			po::options_description& hidden) = 0;
		virtual std::ostream& printToIni(std::ostream& o) const = 0;

		virtual bool renderGui() = 0;

		virtual bool importFromBoost() = 0;

		void addShortCmdAlias(char alias);

		void addHelpText(const std::string& help);

	protected:
		const std::string _cmdName;
		const std::string _cmdDesc;
		const bool _hidden;
		const std::string _guiDesc;
		char _alias = '\0';
		std::string _helpText;

		template<class T>
		void addToCmdBase(po::options_description& visible, po::options_description& hidden, T* p);
	};
	template<class T>
	inline void CmdGuiElement::addToCmdBase(po::options_description& visible, po::options_description& hidden, T* p)
	{
		std::string name = _cmdName;
		if (_alias > 0) {
			name.push_back(',');
			name.push_back(_alias);
		}
		if (_hidden) {
			hidden.add_options()(name.c_str(), po::value<T>(p), "");
		}
		else {
			visible.add_options()(name.c_str(), po::value<T>(p), _cmdDesc.c_str());
		}
	}
	template<>
	inline void CmdGuiElement::addToCmdBase<bool>(po::options_description& visible, po::options_description& hidden, bool* p)
	{
		std::string name = _cmdName;
		if (_alias > 0) {
			name.push_back(',');
			name.push_back(_alias);
		}
		if (_hidden) {
			hidden.add_options()(name.c_str(), po::bool_switch(p), "");
		}
		else {
			visible.add_options()(name.c_str(), po::bool_switch(p), _cmdDesc.c_str());
		}
	}

	//a parameter which is a simple bool, which defaults to false. The presence of a command line argument switches it to true.
	class CheckBox : public CmdGuiElement {
	public:
		CheckBox(const std::string& guiDesc, const std::string& cmdName);
		CheckBox(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription);

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;
		
		bool renderGui() override;
		bool importFromBoost() override;

		bool currentState() const;
		void setState(bool b);

	private:
		bool _state = false;
		bool _boostState = false;
	};

	//a parameter which is a simple bool, but defaults to true. The presence of a command line argument switches it to false
	class InvertedCheckBox : public CmdGuiElement {
	public:
		InvertedCheckBox(const std::string& guiDesc, const std::string& cmdName);
		InvertedCheckBox(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription);

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;
		bool importFromBoost() override;

		bool currentState() const;
		void setState(bool b);

	private:
		bool _state = true;
		bool _boostState = false;
	};

	//a parameter which is a simple bool, but which is selected on the gui as two radio buttons
	class RadioBoolean : public CmdGuiElement {
	public:
		RadioBoolean(const std::string& cmdName,
			const std::string& trueDisplayString, const std::string& falseDisplayString);
		RadioBoolean(const std::string& cmdName,
			const std::string& trueDisplayString, const std::string& falseDisplayString,
			const std::string& cmdDescription);

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;
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

	//a slightly strange class where a pair of booleans on the command line is represented by a single radio in the gui, expressing the combinations of those booleans
	class RadioDoubleBoolean : public CmdGuiElement {
	public:
		RadioDoubleBoolean(const std::string& firstCmdName, const std::string& secondCmdName);
		
		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;
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

	//a parameter which is a number, which can be specified on in a text input in the gui, or on the command line
	class NumericTextBoxWithUnits : public CmdGuiElement {
	public:
		using FloatBuffer = std::array<char, LAPIS_NUMERIC_BUFFER_SIZE>;
		
		NumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName, coord_t defaultValue);
		NumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName, coord_t defaultValue, const std::string& cmdDescription);

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;
		bool importFromBoost() override;

		void updateUnits();
		const char* asText() const;
		coord_t getValueLogErrors() const;

		void copyFrom(const NumericTextBoxWithUnits& other);
		void setValue(coord_t v);
		void setError();

	private:
		coord_t _boostValue;
		FloatBuffer _buffer;

		std::string truncDtoS(coord_t v) const;
	};

	class MultiNumericTextBoxWithUnits : public CmdGuiElement {
	public:
		MultiNumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName, const std::string& defaultValue);
		MultiNumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName, const std::string& defaultValue, const std::string& cmdDescription);

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;
		bool importFromBoost() override;

		void updateUnits();
		const std::vector<coord_t>& cachedValues() const;

	private:
		static constexpr size_t LAPIS_MAX_STRATUM_COUNT = 16;
		std::vector<NumericTextBoxWithUnits> _buffers;
		std::string _boostString;
		std::vector<coord_t> _cachedValues;

		bool _displayWindow = false;

		void _cache();
		void _addBuffer();
		bool _window();
	};

	class IntegerTextBox : public CmdGuiElement {
	public:
		IntegerTextBox(const std::string& guiDesc, const std::string& cmdName, int defaultValue, const std::string& cmdDescription);

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;
		bool importFromBoost() override;

		int getValueLogErrors() const;

	private:
		int _boostValue;
		std::array<char,4> _buffer;
	};

	class TextBox : public CmdGuiElement {
	public:
		TextBox(const std::string& guiDesc, const std::string& cmdName);
		TextBox(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription);

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;

		bool importFromBoost() override;


		std::string getValue() const;

	private:
		static constexpr size_t NAME_BUFFER_SIZE = 31;
		std::array<char, NAME_BUFFER_SIZE> _buffer;
		std::string _boost;
	};

	class FileSpecifierSet : public CmdGuiElement {
	public:
		FileSpecifierSet(const std::string& guiDesc, const std::string& cmdName,
			const std::string& cmdDescription,
			const std::vector<std::string>& wildcards,
			std::unique_ptr<nfdnfilteritem_t>&& fileFilter);

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;

		bool importFromBoost() override;

		const std::set<std::string>& getSpecifiers() const;

	private:
		std::set<std::string> _fileSpecsSet;
		std::vector<std::string> _fileSpecsBoost;
		NFD::UniquePathU8 _nfdFolder;
		NFD::UniquePathSet _nfdFiles;
		bool _recursiveCheck = true;
		std::unique_ptr<nfdnfilteritem_t> _fileFilter;
		std::vector<std::string> _wildcards;
	};

	class FolderTextInput : public CmdGuiElement {
	public:
		FolderTextInput(const std::string& guiDesc, const std::string& cmdName,
			const std::string& cmdDescription);

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;

		bool importFromBoost() override;

		std::filesystem::path path() const;

	private:
		std::string _boostString;
		std::vector<char> _buffer = std::vector<char>(256);
		NFD::UniquePathU8 _nfdFolder;
		std::filesystem::path _cachedPath;
	};

	class CRSInput : public CmdGuiElement {
	public:
		CRSInput(const std::string& guiDesc, const std::string& cmdName, const std::string& defaultDisplay);
		CRSInput(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription, const std::string& defaultDisplay);

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;

		bool importFromBoost() override;

		const CoordRef& cachedCrs() const;
		void renderDisplayString() const;
		void reset();
		void setCrs(const CoordRef& crs);
		void setZUnits(const Unit& zUnits);

	private:
		static constexpr size_t LAPIS_CRS_BUFFER_SIZE = 10000;
		std::vector<char> _buffer = std::vector<char>(LAPIS_CRS_BUFFER_SIZE);
		std::string _displayString;
		CoordRef _cachedCrs;
		NFD::UniquePathU8 _nfdFile;
		std::string _boostString;
		std::string _defaultDisplay;

		void _updateCrsAndDisplayString(const std::string& displayIfEmpty);
	};

	class ClassCheckBoxes : public CmdGuiElement {
	public:
		ClassCheckBoxes(const std::string& cmdName, const std::string& cmdDescription);

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;

		bool importFromBoost() override;

		const std::array<bool, 256>& allChecks() const;
		void setState(size_t idx, bool b);

		std::shared_ptr<LasFilter> getFilter() const;

	private:
		std::array<bool, 256> _checks;
		std::string _boostString;
		std::string _displayString;
		size_t _nChecked = 0;
		bool _displayWindow = false;

		void _updateDisplayString();
		bool _inverseDisplayString() const;
		bool _window();

		const inline static std::vector<std::string> _names = { "Never Classified",
			"Unassigned",
			"Ground",
			"Low Vegetation",
			"Medium Vegetation",
			"High Vegetation",
			"Building",
			"Low Point",
			"Model Key (LAS 1.0-1.3)/Reserved (LAS 1.4)",
			"Water",
			"Rail",
			"Road Surface",
			"Overlap (LAS 1.0-1.3)/Reserved (LAS 1.4)",
			"Wire - Guard (Shield)",
			"Wire - Conductor (Phase)",
			"Transmission Tower",
			"Wire-Structure Connector (Insulator)",
			"Bridge Deck",
			"High Noise" };
	};

	class UnitDecider {
	public:
		int operator()(const std::string& s) const;
		std::string operator()(int i) const;
	};
	class SmoothDecider {
	public:
		int operator()(const std::string& s) const;
		std::string operator()(int i) const;
	};
	class IdAlgoDecider {
	public:
		int operator()(const std::string& s) const;
		std::string operator()(int i) const;
	};
	class SegAlgoDecider {
	public:
		int operator()(const std::string& s) const;
		std::string operator()(int i) const;
	};

	//This class represents a method of selection between multiple options: in the gui, via a radio button, and in the command line, via regex matching with using input
	//DECIDER is a functor class with two overloads of operator():
	//one takes a string as provided by the user on the command line, and converts that to an int representing which radio option is selected
	//the other takes an int representing a radio button, and returns a string suitable which would produce that int in the other function
	template<class DECIDER, class VALUE>
	class RadioSelect : public CmdGuiElement {
	public:
		struct SingleButton {
			std::string display;
			VALUE value;
		};

		RadioSelect(const std::string& guiDesc, const std::string& cmdName);
		RadioSelect(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription);

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;

		bool importFromBoost() override;

		const VALUE& currentSelection() const;
		
		//whichButton should be non-negative. The first call of this function also determines the default value, but otherwise the order of the
		//calls doesn't matter--the buttons are sorted by whichButton, not by the order this function is called
		void addOption(const std::string& display, int whichButton, const VALUE& value);

		void setSingleLine();

	private:
		int _radio = -1;
		std::map<int,SingleButton> _buttons;
		std::string _boostString;

		DECIDER _decider;

		bool _singleLine = false;
	};
	template<class DECIDER, class VALUE>
	inline RadioSelect<DECIDER, VALUE>::RadioSelect(const std::string& guiDesc, const std::string& cmdName)
		: CmdGuiElement(guiDesc, cmdName)
	{
	}
	template<class DECIDER, class VALUE>
	inline RadioSelect<DECIDER, VALUE>::RadioSelect(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription)
		: CmdGuiElement(guiDesc, cmdName, cmdDescription)
	{
	}
	template<class DECIDER, class VALUE>
	inline void RadioSelect<DECIDER, VALUE>::addToCmd(po::options_description& visible, po::options_description& hidden)
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
		ImGui::Text(_guiDesc.c_str());

		if (_helpText.size() && !_singleLine) {
			ImGui::SameLine();
			ImGuiHelpMarker(_helpText.c_str());
		}

		for (auto& v : _buttons) {
			if (_singleLine) {
				ImGui::SameLine();
			}
			changed = ImGui::RadioButton(v.second.display.c_str(), &_radio, v.first) || changed;
		}
		if (_helpText.size() && _singleLine) {
			ImGui::SameLine();
			ImGuiHelpMarker(_helpText.c_str());
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
}

#endif