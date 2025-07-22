#include"Parameter.hpp"

namespace lapis {
    size_t ParameterRegistrar::registerParameter(Parameter* param)
    {
        _params.push_back(param);
        return _params.size() - 1;
    }
    size_t ParameterRegistrar::size() const
    {
        return _params.size();
    }
    ParameterRegistrar& ParameterRegistrar::get() {
        static ParameterRegistrar instance;
        return instance;
    }
    std::vector<Parameter*>::iterator ParameterRegistrar::begin() {
        return _params.begin();
    }
    std::vector<Parameter*>::iterator ParameterRegistrar::end() {
        return _params.end();
    }
}