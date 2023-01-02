#pragma once
#ifndef LP_PARAMETER_H
#define LP_PARAMETER_H

#include"CommonGuiElements.hpp"

//forward declares, including those needed by child classes

namespace lapis {

	enum class ParamCategory {
		data, computer, process
	};

	class Parameter {
	public:

		Parameter() = default;
		virtual ~Parameter() = default;

		virtual void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) = 0;

		virtual std::ostream& printToIni(std::ostream& o) = 0;

		//this function should assume that it's already in the correct tab/child
		//it should feel limited by the value of ImGui::GetContentRegionAvail().x, but not by y
		//if making things look nice takes more vertical space than provided, adjustments should be made in the layout portion of the code
		virtual void renderGui() = 0;

		virtual ParamCategory getCategory() const = 0;

		virtual void importFromBoost() = 0;
		virtual void updateUnits() = 0;

		virtual bool prepareForRun() = 0;
		virtual void cleanAfterRun() = 0;
		virtual void reset() = 0;
	};
}

#endif

	/*
	* 	class NewParameter : public Parameter {
	public:

		NewParameter();

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
		};

		and in the cpp:

		size_t NewParameter::registeredIndex = RunParameters::singleton().registerParameter(new NewParameter());

	*/

