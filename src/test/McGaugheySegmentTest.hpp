#pragma once
#ifndef LP_MCGAUGHEY_SEGMENT_TEST_HPP
#define LP_MCGAUGHEY_SEGMENT_TEST_HPP

#include"test_pch.hpp"
#include"TestUtils.hpp"

namespace lapis {
    SegmentResults applyMcGaugheySegment(const InputData& input, const TestCase& testCase);
}

#endif