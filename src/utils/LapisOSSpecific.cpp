#include"LapisOSSpecific.hpp"
#include<vector>

namespace lapis {
	std::string executableFolder() {
#ifdef _WIN32
		std::vector<char> pathBuf;
		int copied = 0;
		do {
			pathBuf.resize(pathBuf.size() + MAX_PATH);
			copied = GetModuleFileNameA(0, pathBuf.data(), (int)pathBuf.size());
		} while (copied >= pathBuf.size());
		pathBuf.resize(copied);

		return std::string(pathBuf.begin(), pathBuf.end());
#else
		static_assert(false, "Please implement executableFolder() for this OS");
#endif
	}
}
