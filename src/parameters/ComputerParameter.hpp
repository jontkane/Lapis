#pragma once
#ifndef LP_COMPUTERPARAMETER_H
#define LP_COMPUTERPARAMETER_H

#include"Parameter.hpp"

namespace lapis {
	class ComputerParameter : public Parameter {
	public:

		ComputerParameter();

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

		int nThread() const;

	private:
		static int _defaultNThread();

		IntegerTextBox _thread{ "Number of Threads:","thread", _defaultNThread(),
		"The number of threads to run Lapis on. Defaults to the number of cores on the computer" };
		std::string _threadCmd = "thread";
	};
}

#endif