#pragma once
#ifndef LP_HIGH_POINT_TEST_HPP
#define LP_HIGH_POINT_TEST_HPP

#include"test_pch.hpp"
#include"TestUtils.hpp"

namespace lapis {
    VectorDataset<Point> applyHighPoint(const InputData& input, const TestCase& testCase);
}

#endif
