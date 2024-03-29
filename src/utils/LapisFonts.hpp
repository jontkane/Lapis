#pragma once
#ifndef LP_LAPISFONTS_H
#define LP_LAPISFONTS_H

#include"..\imgui\imgui.h"

namespace lapis {
	class LapisFonts {
	public:
		static ImFont* getRegularFont();
		static ImFont* getBoldFont();
		static ImFont* getTitleFont();
		static void initFonts();
	};
}

#endif