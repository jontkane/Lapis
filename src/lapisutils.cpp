#include"app_pch.hpp"
#include"LapisUtils.hpp"

namespace fs = std::filesystem;

namespace lapis {





	bool extentSorter(const Extent& lhs, const Extent& rhs) {
		if (lhs.ymax() != rhs.ymax()) {
			return lhs.ymax() < rhs.ymax();
		}
		if (lhs.xmax() != rhs.xmax()) {
			return lhs.xmax() < rhs.xmax();
		}
		if (lhs.ymin() != rhs.ymin()) {
			return lhs.ymin() < rhs.ymin();
		}
		return lhs.xmin() < rhs.ymin();
	}
	
	std::string insertZeroes(int value, int maxvalue)
	{
		int nDigits = (int)(std::log10(maxvalue) + 1);
		int thisDigitCount = std::max(1, (int)(std::log10(value) + 1));
		return std::string(nDigits - thisDigitCount, '0') + std::to_string(value);
	}


	//returns a useful default for the number of concurrent threads to run
	unsigned int defaultNThread() {
		unsigned int out = std::thread::hardware_concurrency();
		return out > 1 ? out - 1 : 1;
	}
}