#pragma once
#ifndef LP_OUTPUTPARAMETER_H
#define LP_OUTPUTPARAMETER_H

#include"Parameter.hpp"

namespace lapis {
	class OutputParameter : public Parameter {
	public:

		OutputParameter();

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

		const std::filesystem::path& path();
		bool isDebugNoOutput() const;

	private:
		FolderTextInput _output{ "Output Folder:","output",
			"The output folder to store results in" };
		CheckBox _debugNoOutput{ "Debug no Output","debug-no-output" };

		std::filesystem::path _outPath;
		bool _runPrepared = false;
	};
}

#endif