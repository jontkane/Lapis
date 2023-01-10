#pragma once
#ifndef LP_MULTINUMERICTEXTBOXWITHUNITS_H
#define LP_MULTINUMERICTEXTBOXWITHUNITS_H

#include"GuiCmdElement.hpp"


namespace lapis {

	class NumericTextBoxWithUnits;

	class MultiNumericTextBoxWithUnits : public GuiCmdElement {
	public:
		MultiNumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName, const std::string& defaultValue);
		MultiNumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName, const std::string& defaultValue, const std::string& cmdDescription);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;
		bool importFromBoost() override;

		void updateUnits();
		const std::vector<coord_t>& cachedValues() const;

	private:
		static constexpr size_t LAPIS_MAX_STRATUM_COUNT = 16;
		std::list<NumericTextBoxWithUnits> _buffers;
		std::string _boostString;
		std::vector<coord_t> _cachedValues;

		bool _displayWindow = false;

		void _cache();
		void _addBuffer();
		bool _window();
	};
}

#endif