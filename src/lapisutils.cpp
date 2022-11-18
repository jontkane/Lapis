#include"app_pch.hpp"
#include"LapisUtils.hpp"

namespace fs = std::filesystem;

namespace lapis {


	bool operator<(const DemFileAlignment& a, const DemFileAlignment& b) {
		return a.filename < b.filename;
	}

	bool operator<(const LasFileExtent& a, const LasFileExtent& b) {
		if (a.filename == b.filename) { return false; }
		if (a.ext.ymax() > b.ext.ymax()) { return true; }
		if (a.ext.ymax() < b.ext.ymax()) { return false; }
		if (a.ext.ymin() > b.ext.ymin()) { return true; }
		if (a.ext.ymin() < b.ext.ymin()) { return false; }
		if (a.ext.xmax() > b.ext.xmax()) { return true; }
		if (a.ext.xmin() < b.ext.xmin()) { return false; }
		if (a.ext.xmax() > b.ext.xmax()) { return true; }
		if (a.ext.xmax() < b.ext.xmax()) { return false; }
		return a.filename < b.filename;
	}


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
		size_t nDigits = (size_t)(std::log10(maxvalue) + 1);
		size_t thisDigitCount = std::max(1ull, (size_t)(std::log10(value) + 1));
		return std::string(nDigits - thisDigitCount, '0') + std::to_string(value);
	}


	//returns a useful default for the number of concurrent threads to run
	unsigned int defaultNThread() {
		unsigned int out = std::thread::hardware_concurrency();
		return out > 1 ? out - 1 : 1;
	}
}