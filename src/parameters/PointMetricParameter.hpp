#pragma once
#ifndef LP_POINTMETRICPARAMETER_H
#define LP_POINTMETRICPARAMETER_H

#include"Parameter.hpp"

namespace lapis {
	class PointMetricParameter : public Parameter {
	public:

		PointMetricParameter();

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

		bool doAllReturns() const;
		bool doFirstReturns() const;

		const std::vector<coord_t>& strata() const;
		const std::vector<std::string>& strataNames();
		coord_t canopyCutoff() const;
		bool doStratumMetrics() const;
		bool doAdvancedPointMetrics() const;

	private:
		Title _title{ "Point Metric Options" };

		NumericTextBoxWithUnits _canopyCutoff{ "Canopy Cutoff:","canopy",2,
		"The height threshold for a point to be considered canopy." };
		MultiNumericTextBoxWithUnits _strata{ "Stratum Breaks:","strata","0.5,1,2,4,8,16,32,48,64",
			"A comma-separated list of strata breaks on which to calculate strata metrics." };
		RadioBoolean _advMetrics{ "adv-point","All Metrics","Common Metrics",
		"Calculate a larger suite of point metrics." };
		InvertedCheckBox _doStrata{ "Calculate Strata Metrics","skip-strata" };

		static constexpr int FIRST_RETURNS = RadioDoubleBoolean::FIRST;
		static constexpr int ALL_RETURNS = RadioDoubleBoolean::SECOND;
		static constexpr bool DO_SKIP = true;
		static constexpr bool DONT_SKIP = false;
		RadioDoubleBoolean _whichReturns{ "skip-first-returns","skip-all-returns" };

		std::vector<std::string> _strataNames;

		bool _runPrepared = false;
	};
}

#endif