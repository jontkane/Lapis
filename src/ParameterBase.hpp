#pragma once
#ifndef LP_PARAMETER_BASE_H
#define LP_PARAMETER_BASE_H

#include"app_pch.hpp"

namespace lapis {
		//Each of these should correspond to a cohesive unit of the parameters fed into Lapis--not necessarily a single parameter each
		//As a rule of thumb, each one should correspond to any number of parameters that should always be displayed together in the GUI
		enum class ParamName {
		name,
		las,
		dem,
		output,
		lasOverride,
		demOverride,
		outUnits,
		alignment,
		filters,
		computerOptions,
		whichProducts,
		csmOptions,
		pointMetricOptions,
		taoOptions,
		fineIntOptions,
		topoOptions,

		_Count_NotAParam //this element's value will be equal to the cardinality of the enum class
	};

	enum class ParamCategory {
		data, computer, process
	};

	namespace po = boost::program_options;
	class ParamBase {
	public:
		using FloatBuffer = std::array<char, 11>;

		ParamBase() = default;
		virtual ~ParamBase() = default;

		virtual void addToCmd(po::options_description& visible,
			po::options_description& hidden) = 0;

		virtual std::ostream& printToIni(std::ostream& o) = 0;

		//this function should assume that it's already in the correct tab/child
		//it should feel limited by the value of ImGui::GetContentRegionAvail().x, but not by y
		//if making things look nice takes more vertical space than provided, adjustments should be made in the layout portion of the code
		virtual void renderGui() = 0;

		virtual ParamCategory getCategory() const = 0;

		virtual void importFromBoost() = 0;
		virtual void updateUnits() = 0;

		virtual void prepareForRun() = 0;
		virtual void cleanAfterRun() = 0;
	};

	template<ParamName N>
	class Parameter : public ParamBase {};

	//This is the skeleton of a new template specialization of Parameter
	/*
	template<>
	class Parameter<ParamName::newparam> : public ParamBase {
	public:
		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		void prepareForRun() override;
		void cleanAfterRun() override;
	};
	*/
}

#endif