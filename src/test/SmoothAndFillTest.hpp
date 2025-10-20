#pragma once
#ifndef LP_SMOOTH_AND_FILL_TEST_HPP
#define LP_SMOOTH_AND_FILL_TEST_HPP

#include"test_pch.hpp"
#include"TestUtils.hpp"

namespace lapis {
    Raster<csm_t> applySmoothAndFill(const InputData& input, const TestCase& testCase);
}

#endif