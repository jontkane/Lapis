#include"ParameterGetter.hpp"

namespace lapis {
    std::unique_ptr<ParameterManager> pmSingleton;
    void setParameterManager(ParameterManager* pm)
    {
        pmSingleton.reset(pm);
    }
    ParameterManager& parameterManager()
    {
        return *pmSingleton;
    }
}