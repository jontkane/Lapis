#pragma once
#ifndef LP_LAPISGUI_H
#define LP_LAPISGUI_H

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <nfd.hpp>
#include "gis/Unit.hpp"
#include "LapisController.hpp"


namespace lapis {

	//this class is a quick and dirty to get the logger printing to the gui. It will be replaced when the logger is overhauled
	class VectorStream : private std::streambuf, public std::ostream {
	public:
		std::vector<std::string> log;

		VectorStream() : std::ostream(this), log(1) {}
	private:
		int overflow(int c) override {
			if (c == '\n') {
				log.emplace_back();
			}
			else {
				std::stringstream ss{ log[log.size() - 1] };
				ss << (char)c;
			}
			return 0;
		}
	};

	struct LapisGuiObjects {
		std::unique_ptr<LapisController> controller;
		VectorStream vlog;
		std::thread runThread;
		NFD::UniquePathU8 inifile;

		bool showAdvancedDataOptions = false;

		static LapisGuiObjects& getGuiObjects();
		bool isRunning();

	private:
		LapisGuiObjects() = default;
	};

	void renderFullGui();

	void guiTabs();

	void runTab();

	void processingOptions();

	void displayLog();

	ImVec2 getRegionSize(int i);

}

#endif