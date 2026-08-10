#pragma once
#ifndef LP_COMPUTERPARAMETER_H
#define LP_COMPUTERPARAMETER_H

#include"Parameter.hpp"

namespace lapis {
	class ComputerParameter : public Parameter {
	public:

		ComputerParameter();
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

		int nThread() const;
        int concurrentIO() const;

	private:
		static int _defaultNThread();

		Title _title{ "Computer-Specific Options" };

		NumericTextBox _thread{ "Number of Threads:","thread", (double)_defaultNThread(),
		"The number of threads to run Lapis on. Defaults to the number of cores on the computer" };

        NumericTextBox _concurrentIO{ "Number of concurrent read/writes per drive: ", "concurrentIO", 4.0, "The number of concurrent read/write operations per drive. Defaults to 4" };
	};
}

#endif