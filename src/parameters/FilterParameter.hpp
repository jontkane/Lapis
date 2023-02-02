#pragma once
#ifndef LP_FILTERPARAMETER_H
#define LP_FILTERPARAMETER_H

#include"Parameter.hpp"

namespace lapis {
	class MetadataPdf;

	class FilterParameter : public Parameter {
	public:

		FilterParameter();

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
		static size_t parameterRegisteredIndex;

		const std::vector<std::shared_ptr<LasFilter>>& filters();
		coord_t minht() const;
		coord_t maxht() const;

		void describeInPdf(MetadataPdf& pdf);

	private:

		NumericTextBoxWithUnits _minht{ "Minimum Height:","minht",-8,
		"The threshold for low outliers. Points with heights below this value will be excluded." };
		NumericTextBoxWithUnits _maxht{ "Maximum Height:","maxht",100,
		"The threshold for high outliers. Points with heights above this value will be excluded." };
		InvertedCheckBox _filterWithheld{ "Filter Withheld Points","use-withheld" };
		ClassCheckBoxes _class{ "class",
		"A comma-separated list of LAS point classifications to use for this run.\n"
				"Alternatively, preface the list with ~ to specify a blacklist." };

		std::vector<std::shared_ptr<LasFilter>> _filters;

		bool _runPrepared = false;

		void _outlierPdf(MetadataPdf& pdf);
		void _withheldPdf(MetadataPdf& pdf);
		void _classPdf(MetadataPdf& pdf);
	};
}

#endif