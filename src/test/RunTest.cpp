#include"RunTest.hpp"
#include"../parameters/LapisParameters.hpp"
#include"../run/LapisController.hpp"

namespace lapis {
    void runTest(const std::vector<std::string>& argv)
    {
        CoutSuppressor suppressor{};
        setParameterManager(new LapisParameters());
        ParameterManager& pm = parameterManager();
        pm.reset();
        HandlerRegistrar::get().initHandlers();
        pm.parseArgs(argv);
        LapisController controller = LapisController();
        ASSERT_TRUE(controller.processFullArea());
        pm.reset();
    }

    TEST(RunTest, DefaultParamsRegression) {
        //this hasn't been run before, so the first run is generating the output for regression
        TestParameterGetter testParams{ findFileInEitherFolder("ParameterTestValues.yaml") };
        runTest(testParams.getDefaultTestParameters());
    }
}
