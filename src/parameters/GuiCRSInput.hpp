#pragma once
#ifndef LP_GUICRSINPUT_H
#define LP_GUICRSINPUT_H

#include"GuiCmdElement.hpp"

namespace lapis {
	class CRSInput : public GuiCmdElement {
	public:
		CRSInput(const std::string& guiDesc, const std::string& cmdName, const std::string& defaultDisplay);
		CRSInput(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription, const std::string& defaultDisplay);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;

		bool importFromBoost() override;

		const CoordRef& cachedCrs() const;
		void renderDisplayString() const;
		void reset();
		void setCrs(const CoordRef& crs);
		void setZUnits(const LinearUnit& zUnits);

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
}

#endif