#pragma once
#ifndef LP_NAME_PARAMETER_H
#define LP_NAME_PARAMETER_H

#include"Parameter.hpp"

namespace lapis {

	class NameParameter : public Parameter {
	public:

		NameParameter();

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

		const std::string& name();

	private:
		TextBox _name{ "Name of Run:","name",
		"The name of the run. If specified, will be included in the filenames and metadata." };

		std::string _nameString;
		bool _runPrepared = false;
	};
}

#endif