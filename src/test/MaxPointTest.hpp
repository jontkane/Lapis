#pragma once
#ifndef LP_MAX_POINT_TEST_HPP
#define LP_MAX_POINT_TEST_HPP

#include"test_pch.hpp"
#include"TestUtils.hpp"

namespace lapis {
    Raster<csm_t> applyMaxPointRaster(const InputData& input, const TestCase& testCase);
}

#endif