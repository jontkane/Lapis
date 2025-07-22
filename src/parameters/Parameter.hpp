#pragma once
#ifndef LP_PARAMETER_H
#define LP_PARAMETER_H

#include"CommonGuiElements.hpp"

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

	//this class should only be used for static registration of parameters
	//calls to these functions at any other time will not do anything useful, and may break the program
	class ParameterRegistrar {
	public:
		static ParameterRegistrar& get();
		size_t registerParameter(Parameter* param);
		std::vector<Parameter*>::iterator begin();
        std::vector<Parameter*>::iterator end();
        std::vector<Parameter*>::const_iterator begin() const;
        std::vector<Parameter*>::const_iterator end() const;
        
		template<class PARAM>
		PARAM* getParameter();
        template<class PARAM>
		const PARAM* getParameter() const;

		size_t size() const;
	private:
		ParameterRegistrar() = default;
		std::vector<Parameter*> _params;
	};

#define LAPIS_PARAMETER_REGISTER_DECLARE static size_t parameterRegisteredIndex;
#define LAPIS_PARAMETER_REGISTER_DEFINE(PARAMETER) \
	size_t PARAMETER::parameterRegisteredIndex = ParameterRegistrar::get().registerParameter(new PARAMETER());

    template<class PARAM>
	PARAM* ParameterRegistrar::getParameter() {
		return dynamic_cast<PARAM*>(_params[PARAM::parameterRegisteredIndex]);
	}
    template<class PARAM>
	const PARAM* ParameterRegistrar::getParameter() const {
		return dynamic_cast<const PARAM*>(_params[PARAM::parameterRegisteredIndex]);
	}
}

#endif

