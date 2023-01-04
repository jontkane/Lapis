#pragma once
#ifndef LP_TAOPARAMETER_H
#define LP_TAOPARAMETER_H

#include"Parameter.hpp"
#include"..\algorithms\TaoIdAlgorithm.hpp"
#include"..\algorithms\TaoSegmentAlgorithm.hpp"

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

		coord_t minTaoHt() const;
		coord_t minTaoDist() const;

		TaoIdAlgorithm* taoIdAlgo();
		TaoSegmentAlgorithm* taoSegAlgo();

	private:

		NumericTextBoxWithUnits _minht{ "","min-tao-ht",2 };
		NumericTextBoxWithUnits _mindist{ "Minimum Distance Between Trees:","min-tao-dist",0 };

		class IdAlgoDecider {
		public:
			int operator()(const std::string& s) const;
			std::string operator()(int i) const;
		};
		RadioSelect<IdAlgoDecider, IdAlgo::IdAlgo> _idAlgo{ "Tree ID Algorithm:","id-algo" };
		std::unique_ptr<TaoIdAlgorithm> _idAlgorithm;

		class SegAlgoDecider {
		public:
			int operator()(const std::string& s) const;
			std::string operator()(int i) const;
		};
		RadioSelect<SegAlgoDecider, SegAlgo::SegAlgo> _segAlgo{ "Canopy Segmentation Algorithm:","seg-algo" };
		std::unique_ptr<TaoSegmentAlgorithm> _segmentAlgorithm;
		RadioBoolean _sameMinHt{ "tao-same-min-ht","Same as Point Metric Canopy Cutoff","Other:" };

		bool _runPrepared = false;
	};
}

#endif