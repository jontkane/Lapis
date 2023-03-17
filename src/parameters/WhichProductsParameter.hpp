#pragma once
#ifndef LP_WHICHPRODUCTSPARAMETER_H
#define LP_WHICHPRODUCTSPARAMETER_H

#include"Parameter.hpp"

namespace lapis {
	class WhichProductsParameter : public Parameter {
	public:

		WhichProductsParameter();

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

		bool doCsm() const;
		bool doPointMetrics() const;
		bool doTao() const;
		bool doTopo() const;
		bool doFineInt() const;

	private:

		Title _title{ "Product Selection" };

		InvertedCheckBox _doCsm{ "Canopy Surface Model","skip-csm" };
		InvertedCheckBox _doPointMetrics{ "Point Metrics","skip-point-metrics" };
		InvertedCheckBox _doTao{ "Tree ID","skip-tao" };
		InvertedCheckBox _doTopo{ "Topographic Metrics","skip-topo" };
		CheckBox _doFineInt{ "Fine-Scale Intensity Image","fine-int",
		"Create a canopy mean intensity raster with the same resolution as the CSM" };
	};
}

#endif