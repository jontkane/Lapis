#pragma once
#ifndef LP_TAOPARAMETER_H
#define LP_TAOPARAMETER_H

#include"Parameter.hpp"
#include"..\algorithms\TaoIdAlgorithm.hpp"
#include"..\algorithms\TaoSegmentAlgorithm.hpp"
#include"..\algorithms\McGaugheySegment.hpp" // for McGaugheySmoothType

namespace lapis {

	namespace IdAlgo {
		enum IdAlgo {
			OTHER,
			HIGHPOINT,
		};
	}
	namespace SegAlgo {
		enum SegAlgo {
			OTHER,
			WATERSHED,
		};
	}

	class TaoParameter : public Parameter {
	public:

		TaoParameter();
		LAPIS_PARAMETER_REGISTER_DECLARE;

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

		coord_t minTaoHt() const;
		coord_t minTaoDist() const;

		TaoIdAlgorithm* taoIdAlgo();
		const std::vector<std::unique_ptr<TaoSegmentAlgorithm>>& taoSegAlgos();

	private:

		Title _title{ "Tree Identification Options" };

		NumericTextBoxWithUnits _minht{ "","min-tao-ht",2 };
		NumericTextBoxWithUnits _mindist{ "Minimum Distance Between Trees:","min-tao-dist",0 };

		class IdAlgoDecider {
		public:
			int operator()(const std::string& s) const;
			std::string operator()(int i) const;
		};
		RadioSelect<IdAlgoDecider, IdAlgo::IdAlgo> _idAlgo{ "Tree ID Algorithm:","id-algo" };
		std::unique_ptr<TaoIdAlgorithm> _idAlgorithm;

		std::vector<std::unique_ptr<TaoSegmentAlgorithm>> _segmentAlgorithms;
		RadioBoolean _sameMinHt{ "tao-same-min-ht","Same as Point Metric Canopy Cutoff","Other:" };

		CheckBox _doWatershed{ "Watershed", "watershed" };
		CheckBox _vectorizeWatershed{ "Produce Polygons" ,"vectorize-segments" };

        CheckBox _doMcgaughey{ "McGaughey", "do-mcgaughey" };
        NumericTextBox _mcgNvertices{ "Number of Vertices:", "mcg-nvertices", 16 };
        NumericTextBox _mcgSlopechangeMultiplier{ "Slope Change Factor:", "mcg-slope-change-factor", 4 };
        NumericTextBox _mcgHeightCutoffMultiplier{ "Height Cutoff Factor:", "mcg-height-cutoff-factor", 2.0 / 3.0 };
        NumericTextBox _mcgMaxDistMultiplier{ "Maximum Distance Factor:", "mcg-max-dist-factor", 3.0 / 4.0 };
        class McGaugheySmoothDecider {
		public:
			int operator()(const std::string& s) const;
			std::string operator()(int i) const;
        };
        RadioSelect<McGaugheySmoothDecider, McGaugheySmoothType> _mcgSmoothType{ "Smoothing Type:", "mcg-smooth-type" };

		bool _runPrepared = false;
	};
}

#endif