#pragma once
#ifndef LP_GUICMDELEMENT_H
#define LP_GUICMDELEMENT_H

#include"param_pch.hpp"

namespace lapis {
	static constexpr size_t LAPIS_NUMERIC_BUFFER_SIZE = 11;

	/*
	* Implementations of this class contain all the functionality needed to handle the conversion between the command line, gui, and internal data object
	* for a single parameter. Anything that cares about the interaction of multiple command line parameters should be handled at a higher level
	*/
	class GuiCmdElement {
	public:

		GuiCmdElement(const std::string& guiDesc, const std::string& cmdName);
		GuiCmdElement(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription);
		GuiCmdElement(const GuiCmdElement&) = default;

		virtual ~GuiCmdElement() = default;

		virtual void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) = 0;
		virtual std::ostream& printToIni(std::ostream& o) const = 0;

		virtual bool renderGui() = 0;

		virtual bool importFromBoost() = 0;

		void addShortCmdAlias(char alias);

		void addHelpText(const std::string& help);

		void displayHelp();

	protected:
		std::string _cmdName;
		std::string _cmdDesc;
		bool _hidden;
		std::string _guiDesc;
		char _alias = '\0';
		std::string _helpText;

		template<class T>
		void addToCmdBase(BoostOptDesc& visible, BoostOptDesc& hidden, T* p);
	};
	template<class T>
	inline void GuiCmdElement::addToCmdBase(BoostOptDesc& visible, BoostOptDesc& hidden, T* p)
	{
		namespace po = boost::program_options;
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
	inline void GuiCmdElement::addToCmdBase<bool>(BoostOptDesc& visible, BoostOptDesc& hidden, bool* p)
	{
		namespace po = boost::program_options;
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
}

#endif