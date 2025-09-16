#pragma once
#ifndef LP_VENDOR_RASTER_TEST_HPP
#define LP_VENDOR_RASTER_TEST_HPP

#include"test_pch.hpp"
#include"TestUtils.hpp"

namespace lapis {
    std::vector<LasPoint> applyVendorRaster(const InputData& input, const TestCase& testCase);
}

#endif