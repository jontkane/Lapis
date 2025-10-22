#pragma once
#ifndef LP_WATERSHED_SEGMENT_HPP
#define LP_WATERSHED_SEGMENT_HPP

#include"test_pch.hpp"
#include"TestUtils.hpp"

namespace lapis {
    SegmentResults applyWatershedSegment(const InputData& input, const TestCase& testCase);
}


#endif