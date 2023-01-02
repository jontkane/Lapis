#pragma once
#ifndef LP_GUITEXTBOX_H
#define LP_GUITEXTBOX_H

#include"GuiCmdElement.hpp"

namespace lapis {
	class TextBox : public GuiCmdElement {
	public:
		TextBox(const std::string& guiDesc, const std::string& cmdName);
		TextBox(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;

		bool importFromBoost() override;

		std::string getValue() const;

	private:
		static constexpr size_t NAME_BUFFER_SIZE = 31;
		std::array<char, NAME_BUFFER_SIZE> _buffer;
		std::string _boost;
	};
}

#endif