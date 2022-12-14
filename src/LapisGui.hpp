#pragma once
#ifndef LP_LAPISGUI_H
#define LP_LAPISGUI_H

#include"app_pch.hpp"
#include"LapisController.hpp"


namespace lapis {

	struct LapisGuiObjects {
		std::unique_ptr<LapisController> controller;
		std::thread runThread;
		NFD::UniquePathU8 inputIniFile;
		NFD::UniquePathU8 outputIniFile;

		bool showAdvancedDataOptions = false;

		static LapisGuiObjects& getGuiObjects();
		bool isRunning();

		bool displayDataIssuesWindow = false;

	private:
		LapisGuiObjects() = default;
	};

	void renderFullGui();

	void guiTabs();

	void dataTab();

	void runTab();

	void processingOptions();

	void productOptions();

	void displayLog();

	void dataIssuesWindow();

	ImVec2 getRegionSize(int i);

}

#endif