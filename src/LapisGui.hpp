#pragma once
#ifndef LP_LAPISGUI_H
#define LP_LAPISGUI_H

#include"app_pch.hpp"
#include"LapisController.hpp"


namespace lapis {

	struct LapisGuiObjects {
		std::unique_ptr<LapisController> controller;
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