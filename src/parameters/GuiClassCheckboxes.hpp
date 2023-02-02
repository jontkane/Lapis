#pragma once
#ifndef LP_GUICLASSCHECKBOXES_H
#define LP_GUICLASSCHECKBOXES_H

#include"GuiCmdElement.hpp"

namespace lapis {
	class ClassCheckBoxes : public GuiCmdElement {
	public:
		ClassCheckBoxes(const std::string& cmdName, const std::string& cmdDescription);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;

		bool importFromBoost() override;

		const std::array<bool, 256>& allChecks() const;
		void setState(size_t idx, bool b);

		std::shared_ptr<LasFilter> getFilter() const;

		const std::vector<std::string>& classNames() const;

	private:
		std::array<bool, 256> _checks;
		std::string _boostString;
		std::string _displayString;
		size_t _nChecked = 0;
		bool _displayWindow = false;

		void _updateDisplayString();
		bool _inverseDisplayString() const;
		bool _window();
	};
}

#endif