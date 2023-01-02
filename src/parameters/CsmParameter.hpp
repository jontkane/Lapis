#pragma once
#ifndef LP_CSMPARAMETER_H
#define LP_CSMPARAMETER_H

#include"Parameter.hpp"
#include"..\algorithms\CsmAlgorithm.hpp"
#include"..\algorithms\CsmPostProcessor.hpp"

namespace lapis {
	class CsmParameter : public Parameter {
	public:

		CsmParameter();

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

		std::shared_ptr<Alignment> csmAlign();
		bool doCsmMetrics() const;

		CsmAlgorithm* csmAlgorithm();
		CsmPostProcessor* csmPostProcessor();

	private:
		NumericTextBoxWithUnits _footprint{ "Diameter of Pulse Footprint:","footprint",0.4 };
		NumericTextBoxWithUnits _cellsize{ "Cellsize:","csm-cellsize",1,
		"The desired cellsize of the output canopy surface model\n"
			"Defaults to 1 meter" };
		InvertedCheckBox _doMetrics{ "Calculate CSM Metrics","skip-csm-metrics" };

		class SmoothDecider {
		public:
			int operator()(const std::string& s) const;
			std::string operator()(int i) const;
		};
		RadioSelect<SmoothDecider, int> _smooth{ "Smoothing:","smooth" };

		InvertedCheckBox _fill{ "Fill Holes","no-csm-fill" };

		std::shared_ptr<Alignment> _csmAlign;

		bool _runPrepared = false;

		std::unique_ptr<CsmAlgorithm> _csmAlgorithm;
		std::unique_ptr<CsmPostProcessor> _csmPostProcessor;
	};
}

#endif