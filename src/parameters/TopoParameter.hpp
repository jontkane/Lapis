#pragma once
#ifndef LP_TOPOPARAMETER_H
#define LP_TOPOPARAMETER_H

#include"Parameter.hpp"

namespace lapis {
	class TopoParameter : public Parameter {
	public:

		TopoParameter();

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		bool prepareForRun() override;
		void cleanAfterRun() override;

		void reset() override;
		static size_t registeredIndex;
	};
}

#endif