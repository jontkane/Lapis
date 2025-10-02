#pragma once
#ifndef LP_DO_NOTHING_CSM_TEST_HPP
#define LP_DO_NOTHING_CSM_TEST_HPP

#include"test_pch.hpp"
#include"TestUtils.hpp"

namespace lapis {
    Raster<csm_t> applyDoNothingCsm(const InputData& input, const TestCase& testCase);
}


#endif