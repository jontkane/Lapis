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

	class Options;

	struct LapisGuiObjects {

		LapisGuiObjects();

		void importFromCmdOptions(const Options& opt);

		std::set<std::string> lasSpecs;
		std::set<std::string> demSpecs;

		std::array<char, 30> nameBuffer = std::array<char, 30>{0};

		NFD::UniquePathU8 lasFolder;
		NFD::UniquePathSet lasFiles;
		bool lasFolderRecursive = true;

		NFD::UniquePathU8 demFolder;
		NFD::UniquePathSet demFiles;
		bool demFolderRecursive = true;

		NFD::UniquePathU8 outputFolderPath;
		std::vector<char> outputFolderBuffer = std::vector<char>(500);

		std::array<char, 4> nThreadBuffer = std::array<char, 4>();
		bool performance = false;
		bool suppressWarnings = true;

		bool showAdvancedDataOptions = false;

		std::array<Unit, 4> unitRadioOrder;
		std::array<std::string, 4> unitPluralNames = { "Units","Meters","Feet","Feet" };
		struct AdvancedData {
			std::string crsString = "Infer From Files";
			bool crsOverrideWindow = false;
			std::vector<char> crsBuffer = std::vector<char>(10000);
			NFD::UniquePathU8 crsFile;
			int unitsRadio = 0;
		};

		AdvancedData lasAdvancedData;
		AdvancedData demAdvancedData;

		int startOfFrameUserUnits = 0;
		int userUnitsRadio = 1;

		using FloatBuffer = std::array<char, 11>;
		struct ParamsWithUnits {
			
			FloatBuffer cellsizeBuffer = FloatBuffer{0};
			FloatBuffer xResBuffer = FloatBuffer{0};
			FloatBuffer yResBuffer = FloatBuffer{0};
			FloatBuffer xOriginBuffer = FloatBuffer{0};
			FloatBuffer yOriginBuffer = FloatBuffer{0};

			FloatBuffer footprintDiamBuffer = FloatBuffer{0};
			FloatBuffer csmCellsizeBuffer = FloatBuffer{0};

			FloatBuffer canopyBuffer = FloatBuffer{0};
			std::vector<FloatBuffer> strataBuffers;

			FloatBuffer minHtBuffer = FloatBuffer{0};
			FloatBuffer maxHtBuffer = FloatBuffer{0};
		};
		ParamsWithUnits withUnits;

		std::string outCRSString = "Same as LAS Files";
		std::vector<char> outCRSBuffer = std::vector<char>(10000);
		NFD::UniquePathU8 alignFile;
		NFD::UniquePathU8 outCRSFile;

		int smoothRadio = 3;

		std::array<bool, 256> classesCheck = std::array<bool, 256>{true};
		std::string classListStr;
		bool filterWithheldCheck = true;

		bool alignFileError = false;
		bool manualAlignWindow = false;
		bool xyResDiff = false;
		bool xyOriDiff = false;

		bool csmAdvanced = false;
		bool stratumWindow = false;
		bool classWindow = false;

		const std::vector<std::string> classNames = { "Never Classified",
		"Unassigned",
		"Ground",
		"Low Vegetation",
		"Medium Vegetation",
		"High Vegetation",
		"Building",
		"Low Point",
		"Model Key (LAS 1.0-1.3)/Reserved (LAS 1.4)",
		"Water",
		"Rail",
		"Road Surface",
		"Overlap (LAS 1.0-1.3)/Reserved (LAS 1.4)",
		"Wire - Guard (Shield)",
		"Wire - Conductor (Phase)",
		"Transmission Tower",
		"Wire-Structure Connector (Insulator)",
		"Bridge Deck",
		"High Noise"};

		bool advancedpoint = false;
		bool fineint = false;

		NFD::UniquePathU8 inifile;

		LapisController lc;
		std::thread runThread;
		std::stringstream logStream;
	};

	void renderGui();

	void lapisGui(LapisGuiObjects& lgo);

	void convertParams(LapisGuiObjects& lgo);

	void lasFileButtons(LapisGuiObjects& lgo);
	void demFileButtons(LapisGuiObjects& lgo);
	void advancedDataOptions(LapisGuiObjects& lgo);


	void processingOptions(LapisGuiObjects& lgo);

	void filterOptions(LapisGuiObjects& lgo);

	void productOptions(LapisGuiObjects& lgo);

	void classWindow(LapisGuiObjects& lgo);

	void metricOptions(LapisGuiObjects& lgo);

	void stratumWindow(LapisGuiObjects& lgo);

	void cleanStrata(LapisGuiObjects& lgo);

	void csmOptions(LapisGuiObjects& lgo);

	void alignmentFromFile(LapisGuiObjects& lgo);

	void manualAlignmentWindow(LapisGuiObjects& lgo);

	void runTab(LapisGuiObjects& lgo);

	void updateClassListString(LapisGuiObjects& lgo);

	ImVec2 getRegionSize(int i);

}

#endif