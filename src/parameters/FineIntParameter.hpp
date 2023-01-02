#pragma once
#ifndef LP_FINEINTPARAMETER_H
#define LP_FINEINTPARAMETER_H

#include"Parameter.hpp"

namespace lapis {
	class FineIntParameter : public Parameter {
	public:

		FineIntParameter();

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

		std::shared_ptr<Alignment> fineIntAlign();
		coord_t fineIntCutoff() const;

	private:
		NumericTextBoxWithUnits _cellsize{ "","fine-int-cellsize",1 };
		InvertedCheckBox _useCutoff{ "Use Returns Above a Certain Height","fine-int-no-cutoff" };
		NumericTextBoxWithUnits _cutoff{ "","fine-int-cutoff",2 };
		RadioBoolean _sameCellsize{ "fine-int-same-cellsize","Same as CSM","Other:" };
		RadioBoolean _sameCutoff{ "fine-int-same-cutoff","Same as Point Metric Canopy Cutoff","Other:" };

		bool _initialSetup = true;

		std::shared_ptr<Alignment> _fineIntAlign;

		bool _runPrepared = false;
	};
}

#endif