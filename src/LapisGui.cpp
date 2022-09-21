#include"app_pch.hpp"
#include"lapisgui.hpp"
#include<locale>
#include<codecvt>
#include"Options.hpp"
#include"gis/alignment.hpp"

namespace lapis {
	void renderingBoilerplate()
	{
		//the code in this function is copied mostly unedited from the ImGui glfw examples
		if (!glfwInit()) {
			throw std::runtime_error("GLFW error\n");
		}
		const char* glsl_version = "#version 130";
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		GLFWwindow* window = glfwCreateWindow(640, 600, "Lapis", nullptr, nullptr);
		if (window == nullptr) {
			throw std::runtime_error("GLFW error\n");
		}
		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigInputTextCursorBlink = true;
		ImGui::StyleColorsClassic();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init(glsl_version);

		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		LapisGuiObjects lgo;

		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			lapisGui(lgo);

			ImGui::Render();
			int display_w, display_h;
			glfwGetFramebufferSize(window, &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(window);
		}

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();
	}
	void lapisGui(LapisGuiObjects& lgo)
	{
		lgo.startOfFrameUserUnits = lgo.userUnitsRadio;

		constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);

		bool isOpen = true;
		ImGui::Begin("Lapis", &isOpen, flags);


		ImGui::BeginTabBar("MainTabs");

		if (ImGui::BeginTabItem("Run")) {
			runTab(lgo);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Data Options")) {
			ImGui::BeginChild("OutputFolderSelect", ImVec2(ImGui::GetContentRegionAvail().x, 75), true, 0);
			ImGui::Text("Output Folder:");
			ImGui::InputText("",lgo.outputFolderBuffer.data(),lgo.outputFolderBuffer.size());
			if (ImGui::Button("Browse")) {
				NFD::PickFolder(lgo.outputFolderPath);
			}
			if (lgo.outputFolderPath) {
				strncpy_s(lgo.outputFolderBuffer.data(), lgo.outputFolderBuffer.size(), lgo.outputFolderPath.get(), lgo.outputFolderBuffer.size());
				lgo.outputFolderPath.reset();
			}
			ImGui::EndChild();

			ImGui::BeginChild("LasButtonsChild", ImVec2(ImGui::GetContentRegionMax().x * 0.5f - 2, 260), true, 0);
			lasFileButtons(lgo);
			ImGui::EndChild();
			ImGui::SameLine();
			ImGui::BeginChild("DemButtonsChild", ImVec2(ImGui::GetContentRegionMax().x * 0.5f - 2, 260), true, 0);
			demFileButtons(lgo);
			ImGui::EndChild();
			advancedDataOptions(lgo);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Computer Options")) { 
			ImGui::Text("Number of threads:");
			ImGui::SameLine();
			ImGui::InputText("", lgo.nThreadBuffer.data(),lgo.nThreadBuffer.size(),ImGuiInputTextFlags_CharsDecimal);
			ImGui::Checkbox("Performance Mode", &lgo.performance);
			ImGui::Checkbox("Suppress GDAL and Proj warnings", &lgo.suppressWarnings);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Processing Options")) { 
			processingOptions(lgo);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();

		ImGui::End();

		if (lgo.startOfFrameUserUnits != lgo.userUnitsRadio) {
			convertParams(lgo);
		}
	}

	void unitsRadioButtons(const char* text, int* selection) {
		ImGui::Text(text);
		ImGui::RadioButton("Infer from files", selection, 0);
		ImGui::RadioButton("Meters", selection, 1);
		ImGui::RadioButton("International Feet", selection, 2);
		ImGui::RadioButton("US Survey Feet", selection, 3);
	}
	void convertParams(LapisGuiObjects& lgo) {

		auto convertBuffer = [&](LapisGuiObjects::FloatBuffer& buff) {
			Unit src = lgo.unitRadioOrder[lgo.startOfFrameUserUnits];
			Unit dst = lgo.unitRadioOrder[lgo.userUnitsRadio];
			try {
				double v = atof(buff.data());
				v = convertUnits(v, src, dst);
				std::string s = std::to_string(v);
				s.erase(s.find_last_not_of('0') + 1, std::string::npos);
				s.erase(s.find_last_not_of('.') + 1, std::string::npos);
				strncpy_s(buff.data(), buff.size(), s.c_str(), buff.size());
			}
			catch (...) {
				const char* msg = "Error";
				strncpy_s(buff.data(), buff.size(), msg, buff.size());
			}
		};
		auto& wu = lgo.withUnits;
		convertBuffer(wu.canopyBuffer);
		convertBuffer(wu.cellsizeBuffer);
		convertBuffer(wu.csmCellsizeBuffer);
		convertBuffer(wu.footprintDiamBuffer);
		convertBuffer(wu.maxHtBuffer);
		convertBuffer(wu.minHtBuffer);
		for (size_t i = 0; i < wu.strataBuffers.size(); ++i) {
			convertBuffer(wu.strataBuffers[i]);
		}
		convertBuffer(wu.xOriginBuffer);
		convertBuffer(wu.yOriginBuffer);
		convertBuffer(wu.xResBuffer);
		convertBuffer(wu.yResBuffer);
	}

	//data options tab
	void lasFileButtons(LapisGuiObjects& lgo)
	{

		std::unique_ptr<nfdnfilteritem_t> lasFileFilter = std::make_unique<nfdnfilteritem_t>(L"LAS Files", L"las,laz");
		if (ImGui::Button("Add Laz Files")) {
			NFD::OpenDialogMultiple(lgo.lasFiles, lasFileFilter.get(), 1, (const nfdnchar_t*)nullptr);
		}
		nfdpathsetsize_t selectedFileCount = 0;
		if (lgo.lasFiles) {
			NFD::PathSet::Count(lgo.lasFiles, selectedFileCount);
			for (nfdpathsetsize_t i = 0; i < selectedFileCount; ++i) {
				NFD::UniquePathSetPathU8 path;
				NFD::PathSet::GetPath(lgo.lasFiles, i, path);
				lgo.lasSpecs.insert(path.get());
			}
			lgo.lasFiles.reset();
		}

		ImGui::SameLine();
		if (ImGui::Button("Add Laz Folder")) {
			NFD::PickFolder(lgo.lasFolder);
		}
		if (lgo.lasFolder) {
			if (lgo.lasFolderRecursive) {
				lgo.lasSpecs.insert(lgo.lasFolder.get());
			}
			else {
				lgo.lasSpecs.insert(lgo.lasFolder.get() + std::string("\\*.las"));
				lgo.lasSpecs.insert(lgo.lasFolder.get() + std::string("\\*.laz"));
			}
			lgo.lasFolder.reset();
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove All")) {
			lgo.lasSpecs.clear();
		}
		//ImGui::SameLine();
		ImGui::Checkbox("Add subfolders", &lgo.lasFolderRecursive);

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
		ImGui::BeginChild("LasFileChild", ImVec2(ImGui::GetContentRegionAvail().x-2, ImGui::GetContentRegionAvail().y ), true, window_flags);
		for (auto& s : lgo.lasSpecs) {
			ImGui::Text(s.c_str());
		}
		ImGui::EndChild();
	}
	void demFileButtons(LapisGuiObjects& lgo) {

		if (ImGui::Button("Add DEM Files")) {
			NFD::OpenDialogMultiple(lgo.demFiles, nullptr, 0, (const nfdnchar_t*)nullptr);
		}
		nfdpathsetsize_t selectedFileCount = 0;
		if (lgo.demFiles) {
			NFD::PathSet::Count(lgo.demFiles, selectedFileCount);
			for (nfdpathsetsize_t i = 0; i < selectedFileCount; ++i) {
				NFD::UniquePathSetPathU8 path;
				NFD::PathSet::GetPath(lgo.demFiles, i, path);
				lgo.demSpecs.insert(path.get());
			}
			lgo.demFiles.reset();
		}

		ImGui::SameLine();
		if (ImGui::Button("Add DEM Folder")) {
			NFD::PickFolder(lgo.demFolder);
		}
		if (lgo.demFolder) {
			if (lgo.demFolderRecursive) {
				lgo.demSpecs.insert(lgo.demFolder.get());
			}
			else {
				lgo.demSpecs.insert(lgo.demFolder.get() + std::string("\\*.*"));
			}
			lgo.demFolder.reset();
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove All")) {
			lgo.demSpecs.clear();
		}
		//ImGui::SameLine();
		ImGui::Checkbox("Add subfolders", &lgo.demFolderRecursive);

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
		ImGui::BeginChild("DemFileChild", ImVec2(ImGui::GetContentRegionAvail().x-2, ImGui::GetContentRegionAvail().y), true, window_flags);
		for (auto& s : lgo.demSpecs) {
			ImGui::Text(s.c_str());
		}
		ImGui::EndChild();
	}
	void advancedDataOptions(LapisGuiObjects& lgo)
	{
		bool textSiezeFocus = false;
		Options& opt = Options::getOptionsObject();
		ImGui::Checkbox("Show Advanced Options", &lgo.showAdvancedDataOptions);
		if (lgo.showAdvancedDataOptions) {
			ImGui::BeginChild("advancedLasOptChild",ImVec2(ImGui::GetContentRegionAvail().x*0.5f,ImGui::GetContentRegionAvail().y),false,0);
			ImGui::Text("LAS CRS:");
			ImGui::BeginChild("LasCRSText", ImVec2(ImGui::GetContentRegionAvail().x, 40), true, ImGuiWindowFlags_HorizontalScrollbar);
			ImGui::Text(lgo.lasAdvancedData.crsString.c_str());
			ImGui::EndChild();
			if (ImGui::Button("Specify CRS")) {
				lgo.lasAdvancedData.crsOverrideWindow = true;
				textSiezeFocus = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset")) {
				lgo.lasAdvancedData.crsString = "Infer From Files";
			}
			unitsRadioButtons("LAS Elevation Units:", &lgo.lasAdvancedData.unitsRadio);
			ImGui::EndChild();

			ImGui::SameLine();
			ImGui::BeginChild("advancedDemOptChild", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, 0);
			ImGui::Text("DEM CRS:");
			ImGui::BeginChild("DemCRSText", ImVec2(ImGui::GetContentRegionAvail().x, 40), true, ImGuiWindowFlags_HorizontalScrollbar);
			ImGui::Text(lgo.demAdvancedData.crsString.c_str());
			ImGui::EndChild();
			if (ImGui::Button("Specify CRS")) {
				lgo.demAdvancedData.crsOverrideWindow = true;
				textSiezeFocus = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset")) {
				lgo.demAdvancedData.crsString = "Infer From Files";
			}
			unitsRadioButtons("DEM Elevation Units:", &lgo.demAdvancedData.unitsRadio);
			ImGui::EndChild();

			auto crsSelectFunc = [](LapisGuiObjects::AdvancedData& ad, std::string name, bool* textFocus) {
				if (ad.crsOverrideWindow) {
					ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
					ImGui::SetNextWindowSize(ImVec2(400, 220));
					if (!ImGui::Begin((name + " CRS Override").c_str(), &ad.crsOverrideWindow, flags)) {
						ImGui::End();
					}
					else {
						ImGuiInputTextFlags txtflag = ImGuiInputTextFlags_EnterReturnsTrue;
						if (*textFocus) {
							ImGui::SetKeyboardFocusHere();
							*textFocus = false;
						}

						bool hitEnter = ImGui::InputTextMultiline("", ad.crsBuffer.data(), ad.crsBuffer.size(), ImVec2(350, 100), txtflag);
						ImGui::Text("The CRS deduction is flexible but not perfect.\nFor best results when manually specifying,\nspecify an EPSG code or a WKT string");
						if (ImGui::Button("Specify from file")) {
							NFD::OpenDialog(ad.crsFile);
						}
						if (ad.crsFile) {
							try {
								std::string wkt = CoordRef(ad.crsFile.get()).getCompleteWKT();
								strncpy_s(ad.crsBuffer.data(), ad.crsBuffer.size(), wkt.c_str(), wkt.size());
							}
							catch (UnableToDeduceCRSException e) {
								const char* crsErrMsg = "Error reading CRS from that file";
								const size_t errMsgLen = strlen(crsErrMsg);
								strncpy_s(ad.crsBuffer.data(), ad.crsBuffer.size(), crsErrMsg, errMsgLen);
							}
							ad.crsFile.reset();
						}
						ImGui::SameLine();
						if (ImGui::Button("OK") || hitEnter) {
							try {
								CoordRef crs = CoordRef(ad.crsBuffer.data());
								ad.crsString = crs.getSingleLineWKT();
								if (crs.isEmpty()) {
									ad.crsString = "Infer From Files";
								}
							}
							catch (UnableToDeduceCRSException e) {
								ad.crsString = "Error inferring CRS from user input";
							}
							ad.crsOverrideWindow = false;
						}
						ImGui::End();
					}
				}
			};

			crsSelectFunc(lgo.lasAdvancedData, "LAS", &textSiezeFocus);
			crsSelectFunc(lgo.demAdvancedData, "DEM", &textSiezeFocus);
		}
	}

	//processing options tab
	void processingOptions(LapisGuiObjects& lgo) {
		auto& wu = lgo.withUnits;

		ImGui::BeginChild("User Units", getRegionSize(0), true, 0);
		ImGui::Text("Output Units");
		ImGui::RadioButton("Meters", &lgo.userUnitsRadio, 1);
		ImGui::RadioButton("International Feet", &lgo.userUnitsRadio, 2);
		ImGui::RadioButton("US Survey Feet", &lgo.userUnitsRadio, 3);
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("Alignment", getRegionSize(1), true, 0);
		ImGui::Text("Output Alignment");
		if (ImGui::Button("Specify From File")) {
			NFD::OpenDialog(lgo.alignFile);
		}

		alignmentFromFile(lgo);
		if (lgo.alignFileError) {
			ImGui::OpenPopup("Alignment File Error");
		}
		ImGui::SetNextWindowSize(ImVec2(300, 300));
		if (ImGui::BeginPopup("Alignment File Error")) {
			lgo.alignFileError = false;
			ImGui::Text("Error opening raster file");
			if (ImGui::Button("OK")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Specify Manually")) {
			lgo.manualAlignWindow = true;
		}
		manualAlignmentWindow(lgo);
		ImGui::Text("Output CRS:");
		ImGui::BeginChild("OutCRSText", ImVec2(ImGui::GetContentRegionAvail().x, 40), true, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::Text(lgo.outCRSString.c_str());
		ImGui::EndChild();
		ImGui::Text("Cellsize: ");
		ImGui::SameLine();
		ImGui::Text(wu.cellsizeBuffer.data());
		ImGui::SameLine();
		if (atof(wu.cellsizeBuffer.data()) != 0) {
			ImGui::Text(lgo.unitPluralNames[lgo.userUnitsRadio].c_str());
		}

		ImGui::EndChild();

		csmOptions(lgo);
		ImGui::SameLine();
		metricOptions(lgo);

		filterOptions(lgo);
		productOptions(lgo);

	}
	void filterOptions(LapisGuiObjects& lgo) {
		auto& wu = lgo.withUnits;

		ImGui::BeginChild("FilterChild", getRegionSize(4), true, 0);
		ImGui::Text("Filter Options");
		ImGui::Checkbox("Use Only First Returns", &lgo.firstReturnsCheck);
		ImGui::Checkbox("Exclude Withheld Points", &lgo.filterWithheldCheck);

		ImGui::Text("Low Outlier:");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::InputText("##minht", wu.minHtBuffer.data(), wu.minHtBuffer.size());
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Text(lgo.unitPluralNames[lgo.userUnitsRadio].c_str());

		ImGui::Text("High Outlier:");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::InputText("##maxht", wu.maxHtBuffer.data(), wu.maxHtBuffer.size());
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Text(lgo.unitPluralNames[lgo.userUnitsRadio].c_str());

		ImGui::Text("Use Classes:");
		ImGui::SameLine();
		ImGui::BeginChild("##classlist", ImVec2(120, 50), true, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::Text(lgo.classListStr.c_str());
		ImGui::EndChild();
		ImGui::SameLine();
		if (ImGui::Button("Change")) {
			lgo.classWindow = true;
		}

		ImGui::EndChild();

		classWindow(lgo);
	}
	void productOptions(LapisGuiObjects& lgo) {
		ImGui::SameLine();
		ImGui::BeginChild("productschild", getRegionSize(5), true, 0);
		ImGui::Text("Optional Product Options");
		ImGui::Checkbox("Advanced Point Metrics", &lgo.advancedpoint);
		ImGui::Checkbox("Fine-scale Canopy Intensity", &lgo.fineint);
		ImGui::EndChild();
	}
	void classWindow(LapisGuiObjects& lgo) {
		if (!lgo.classWindow) {
			return;
		}
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
		ImGui::SetNextWindowSize(ImVec2(300, 400));
		if (!ImGui::Begin("##classwindow", &lgo.classWindow, flags)) {
			ImGui::End();
		}
		if (ImGui::Button("Select All")) {
			for (size_t i = 0; i < lgo.classesCheck.size(); ++i) {
				lgo.classesCheck[i] = true;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Deselect All")) {
			for (size_t i = 0; i < lgo.classesCheck.size(); ++i) {
				lgo.classesCheck[i] = false;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("OK")) {
			lgo.classWindow = false;
			updateClassListString(lgo);
		}
		ImGui::BeginChild("##classcheckboxchild",ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y-60), true, 0);
		for (size_t i = 0; i < lgo.classesCheck.size(); ++i) {
			std::string label = std::to_string(i);
			if (i < lgo.classNames.size()) {
				label += " " + lgo.classNames[i];
			}
			ImGui::Checkbox(label.c_str(), &lgo.classesCheck[i]);
		}
		ImGui::EndChild();
		ImGui::Text("Lapis does not classify points.\nThis filter only applies to\nclassification already performed\non the data.");

		ImGui::End();
	}
	void metricOptions(LapisGuiObjects& lgo) {
		auto& wu = lgo.withUnits;
		ImGui::BeginChild("MetricOptionChild", getRegionSize(3), true, 0);

		ImGui::Text("Metric Options");
		ImGui::Text("Canopy Cutoff:");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::InputText("##canopy", wu.canopyBuffer.data(), wu.canopyBuffer.size());
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Text(lgo.unitPluralNames[lgo.userUnitsRadio].c_str());

		ImGui::BeginChild("stratumleft", ImVec2(ImGui::GetContentRegionAvail().x*0.5f - 2, ImGui::GetContentRegionAvail().y), false, 0);
		ImGui::Text("Stratum Breaks:");
		if (ImGui::Button("Change")) {
			lgo.stratumWindow = true;
		}
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("stratumright", ImVec2(ImGui::GetContentRegionAvail().x - 2, ImGui::GetContentRegionAvail().y), true, 0);
		for (size_t i = 0; i < lgo.withUnits.strataBuffers.size(); ++i) {
			ImGui::Text(lgo.withUnits.strataBuffers[i].data());
			ImGui::SameLine();
			ImGui::Text(lgo.unitPluralNames[lgo.userUnitsRadio].c_str());
		}
		ImGui::EndChild();

		ImGui::EndChild();

		stratumWindow(lgo);
	}
	void stratumWindow(LapisGuiObjects& lgo) {
		auto& strata = lgo.withUnits.strataBuffers;
		if (!lgo.stratumWindow) {
			return;
		}
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
		if (!ImGui::Begin("stratumwindow", &lgo.stratumWindow, flags)) {
			ImGui::End();
		}
		if (ImGui::Button("Add")) {
			strata.emplace_back();
		}
		if (ImGui::Button("Remove All")) {
			strata.clear();
			strata.emplace_back();
		}
		ImGui::SameLine();
		if (ImGui::Button("OK")) {
			cleanStrata(lgo);
			lgo.stratumWindow = false;
		}

		for (size_t i = 0; i < strata.size(); ++i) {
			ImGui::InputText(("##" + std::to_string(i)).c_str(), strata[i].data(), strata[i].size());
			ImGui::SameLine();
			ImGui::Text(lgo.unitPluralNames[lgo.userUnitsRadio].c_str());
		}

		ImGui::End();
	}
	void cleanStrata(LapisGuiObjects& lgo)
	{
		auto& strataStr = lgo.withUnits.strataBuffers;
		std::set<double> numericBreaks;
		for (size_t i = 0; i < strataStr.size(); ++i) {
			numericBreaks.insert(atof(strataStr[i].data()));
		}
		strataStr.clear();
		for (double v : numericBreaks) {
			if (v > 0) {
				strataStr.emplace_back();
				std::string s = std::to_string(v);
				s.erase(s.find_last_not_of('0') + 1, std::string::npos);
				s.erase(s.find_last_not_of('.') + 1, std::string::npos);
				strcpy_s(strataStr[strataStr.size() - 1].data(), strataStr[strataStr.size() - 1].size(), s.c_str());
			}
		}
	}
	void csmOptions(LapisGuiObjects& lgo) {
		auto& wu = lgo.withUnits;
		ImGui::BeginChild("CMS child", getRegionSize(2), true, 0);

		ImGui::Text("Canopy Surface Model Options");

		ImGui::Text("Cellsize:");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::InputText("##CsmCellsize", wu.csmCellsizeBuffer.data(), wu.csmCellsizeBuffer.size(), ImGuiInputTextFlags_CharsDecimal);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Text(lgo.unitPluralNames[lgo.userUnitsRadio].c_str());

		ImGui::Text("Smoothing:");
		ImGui::SameLine();
		ImGui::RadioButton("None", &lgo.smoothRadio, 1);
		ImGui::SameLine();
		ImGui::RadioButton("3x3", &lgo.smoothRadio, 3);
		ImGui::SameLine();
		ImGui::RadioButton("5x5", &lgo.smoothRadio, 5);

		ImGui::Checkbox("Show Advanced Options",&lgo.csmAdvanced);
		if (lgo.csmAdvanced) {
			ImGui::Text("Footprint Diameter:");
			ImGui::SameLine();
			ImGui::PushItemWidth(100);
			ImGui::InputText("##Footprint", wu.footprintDiamBuffer.data(), wu.footprintDiamBuffer.size());
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::Text(lgo.unitPluralNames[lgo.userUnitsRadio].c_str());
		}

		ImGui::EndChild();
	}
	void alignmentFromFile(LapisGuiObjects& lgo) {
		auto& wu = lgo.withUnits;

		if (lgo.alignFile) {
			try {
				Alignment a{ lgo.alignFile.get() };
				lgo.outCRSString = a.crs().getSingleLineWKT();

				Unit alignUnits = a.crs().getXYUnits();
				Unit& userUnits = lgo.unitRadioOrder[lgo.userUnitsRadio];

				auto convertValue = [&](coord_t x) {
					return convertUnits(x, alignUnits, userUnits);
				};

				coord_t xres = convertValue(a.xres());
				coord_t yres = convertValue(a.yres());
				coord_t xorigin = convertValue(a.xOrigin());
				coord_t yorigin = convertValue(a.yOrigin());

				auto moveToBuff = [](coord_t x, LapisGuiObjects::FloatBuffer& buffer) {
					std::string s = std::to_string(x);
					const char* str = s.c_str();
					strcpy_s(buffer.data(), buffer.size(), str);
				};

				moveToBuff(xres, wu.xResBuffer);
				moveToBuff(yres, wu.yResBuffer);
				moveToBuff(xorigin, wu.xOriginBuffer);
				moveToBuff(yorigin, wu.yOriginBuffer);

				if (std::abs(xres-yres)<LAPIS_EPSILON) {
					moveToBuff(xres, wu.cellsizeBuffer);
				}
				else {
					strcpy_s(wu.cellsizeBuffer.data(), wu.cellsizeBuffer.size(), "Diff. X/Y");
				}
			}
			catch (InvalidRasterFileException e) {
				lgo.alignFileError = true;
			}
			catch (InvalidAlignmentException e) { lgo.alignFileError = true; }
			
			lgo.alignFile.reset();
		}
	}
	void manualAlignmentWindow(LapisGuiObjects& lgo) {
		auto& wu = lgo.withUnits;
		if (!lgo.manualAlignWindow) {
			return;
		}
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
		if (!ImGui::Begin("Manual Alignment Window", &lgo.manualAlignWindow,flags)) {
			ImGui::End();
		}

		ImGui::Checkbox("Different X/Y Resolution", &lgo.xyResDiff);
		ImGui::Checkbox("Different X/Y Origin", &lgo.xyOriDiff);

		auto input = [&](const char* name, LapisGuiObjects::FloatBuffer& buff) {
			bool edited = false;
			ImGui::Text(name);
			ImGui::SameLine();
			std::string label = "##" + std::string(name);
			edited = ImGui::InputText(label.c_str(), buff.data(), buff.size(), ImGuiInputTextFlags_CharsDecimal);
			ImGui::SameLine();
			ImGui::Text(lgo.unitPluralNames[lgo.userUnitsRadio].c_str());
			return edited;
		};

		bool cellSizeUpdate = false;
		if (lgo.xyResDiff) {
			cellSizeUpdate = input("X Resolution:", wu.xResBuffer) || cellSizeUpdate;
			cellSizeUpdate = input("Y Resolution:", wu.yResBuffer) || cellSizeUpdate;
			if (cellSizeUpdate) {
				if (atof(wu.xResBuffer.data())==atof(wu.yResBuffer.data())) {
					strcpy_s(wu.cellsizeBuffer.data(), wu.cellsizeBuffer.size(), wu.xResBuffer.data());
				}
				else {
					strcpy_s(wu.cellsizeBuffer.data(), wu.cellsizeBuffer.size(), "Diff. X/Y");
				}
			}
		}
		else {
			cellSizeUpdate = input("X/Y Resolution:", wu.cellsizeBuffer);
			if (cellSizeUpdate) {
				strcpy_s(wu.xResBuffer.data(), wu.xResBuffer.size(), wu.cellsizeBuffer.data());
				strcpy_s(wu.yResBuffer.data(), wu.yResBuffer.size(), wu.cellsizeBuffer.data());
			}
		}

		if (lgo.xyOriDiff) {
			input("X Origin:", wu.xOriginBuffer);
			input("Y Origin:", wu.yOriginBuffer);
		}
		else {
			if (input("X/Y Origin:", wu.xOriginBuffer)) {
				strcpy_s(wu.yOriginBuffer.data(), wu.yOriginBuffer.size(), wu.xOriginBuffer.data());
			}
		}

		ImGui::Text("CRS Input:");
		ImGui::InputTextMultiline("", lgo.outCRSBuffer.data(), lgo.outCRSBuffer.size(),
			ImVec2(ImGui::GetContentRegionAvail().x,300));
		ImGui::Text("The CRS deduction is flexible but not perfect.\nFor best results when manually specifying,\nspecify an EPSG code or a WKT string");
		if (ImGui::Button("Specify CRS From File")) {
			NFD::OpenDialog(lgo.outCRSFile);
		}
		if (lgo.outCRSFile) {
			try {
				CoordRef crs{ lgo.outCRSFile.get() };
				strcpy_s(lgo.outCRSBuffer.data(), lgo.outCRSBuffer.size(), crs.getCompleteWKT().c_str());
			}
			catch (UnableToDeduceCRSException e) {
				lgo.outCRSString = "Error";
				strcpy_s(lgo.outCRSBuffer.data(), lgo.outCRSBuffer.size(), "Error");
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			lgo.outCRSString = "Same as LAS Files";
			lgo.outCRSBuffer = std::vector<char>(lgo.outCRSBuffer.size());
		}
		if (ImGui::Button("OK")) {
			lgo.manualAlignWindow = false;
			try {
				CoordRef crs{ lgo.outCRSBuffer.data() };
				lgo.outCRSString = crs.getSingleLineWKT();
			}
			catch (UnableToDeduceCRSException e) {
				lgo.outCRSString = "Error inferring CRS from user input";
			}
		}
		

		ImGui::End();
	}

	void runTab(LapisGuiObjects& lgo) {
		ImGui::Text("Run Name:");
		ImGui::SameLine();
		ImGui::InputText("##runname", lgo.nameBuffer.data(), lgo.nameBuffer.size());
		if (ImGui::Button("Import Parameters")) {
			std::unique_ptr<nfdu8filteritem_t> iniFileFilter = std::make_unique<nfdu8filteritem_t>("ini files", "ini");
			NFD::OpenDialog(lgo.inifile, iniFileFilter.get(),1);
		}

		if (lgo.inifile) {
			Options& opt = Options::getOptionsObject();
			parseIni(lgo.inifile.get());
			lgo.importFromCmdOptions(opt);
			lgo.inifile.reset();
		}

		ImGui::SameLine();
		if (ImGui::Button("Check Parameters for Issues")) {}
		ImGui::SameLine();
		if (ImGui::Button("Check Data for Issues")) {};
	}

	void updateClassListString(LapisGuiObjects& lgo) {
		lgo.classListStr = "";
		for (size_t i = 0; i < lgo.classesCheck.size(); ++i) {
			if (lgo.classesCheck[i]) {
				if (lgo.classListStr.size()) {
					lgo.classListStr += ",";
				}
				lgo.classListStr += std::to_string(i);
			}
		}
	}

	ImVec2 getRegionSize(int i) {
		float x = 0; float y = 0;
		if (i % 2) {
			x = ImGui::GetContentRegionAvail().x - 2;
		}
		else {
			x = ImGui::GetContentRegionAvail().x * 0.5f - 2;
		}
		if (i / 2 == 0) {
			y = ImGui::GetContentRegionAvail().y / 3.f - 2;
		}
		else if (i / 2 == 1) {
			y = ImGui::GetContentRegionAvail().y / 2.f - 2;
		}
		else {
			y = ImGui::GetContentRegionAvail().y - 2;
		}
		return ImVec2(x, y);
	}

	LapisGuiObjects::LapisGuiObjects()
	{
		unsigned int defnt = defaultNThread() > 1000 ? 999 : defaultNThread(); //I'm not sure if this will ever be run on a thousand core supercomputer, but, I only give 3 characters for entry in the buffer
		strcpy_s(nThreadBuffer.data(), nThreadBuffer.size(), std::to_string(defnt).c_str());
		unitRadioOrder = { linearUnitDefs::unkLinear,linearUnitDefs::meter,linearUnitDefs::foot,linearUnitDefs::surveyFoot };

		auto& wu = withUnits;
		auto init = [](FloatBuffer& buff, const char* value) {
			strcpy_s(buff.data(), buff.size(), value);
		};

		init(wu.canopyBuffer, "2");
		init(wu.cellsizeBuffer, "30");
		init(wu.csmCellsizeBuffer, "1");
		init(wu.footprintDiamBuffer, "0.4");
		init(wu.maxHtBuffer, "100");
		init(wu.minHtBuffer, "-2");
		init(wu.xOriginBuffer, "0");
		init(wu.yOriginBuffer, "0");
		init(wu.xResBuffer, "30");
		init(wu.yResBuffer, "30");

		std::vector<const char*> defStrata = { "0.5","1","2","4","8","16","32","48","64" };
		for (size_t i = 0; i < defStrata.size(); ++i) {
			wu.strataBuffers.emplace_back();
			init(wu.strataBuffers[i], defStrata[i]);
		}

		for (size_t i = 0; i < classesCheck.size(); ++i) {
			classesCheck[i] = true;
		}
		classesCheck[7] = false; classesCheck[9] = false; classesCheck[18] = false;

		updateClassListString(*this);
	}

	void LapisGuiObjects::importFromCmdOptions(const Options& opt)
	{
		namespace pn = paramNames;
		
		lasSpecs.clear();
		for (auto& s : opt.getLas()) {
			lasSpecs.insert(s);
		}
		demSpecs.clear();
		for (auto& s : opt.getDem()) {
			demSpecs.insert(s);
		}

		strcpy_s(nameBuffer.data(), nameBuffer.size(), opt.getName().c_str());
		strcpy_s(outputFolderBuffer.data(), outputFolderBuffer.size(), opt.getOutput().c_str());
		int nThread = opt.getNThread();
		nThread = nThread > 999 ? 999 : nThread;
		strcpy_s(nThreadBuffer.data(), nThreadBuffer.size(), std::to_string(nThread).c_str());
		performance = opt.getPerformanceFlag();
		suppressWarnings = opt.getGdalProjWarningFlag();

		std::unordered_map<std::string, int> unitNameToRadio;
		unitNameToRadio.emplace("unknown", 0);
		unitNameToRadio.emplace("metre", 1);
		unitNameToRadio.emplace("foot", 2);
		unitNameToRadio.emplace("US survey foot", 3);

		auto& lad = lasAdvancedData;
		CoordRef lasCrs = opt.getLasCrs();
		if (!lasCrs.isEmpty()) {
			lad.crsString = lasCrs.getSingleLineWKT();
			strcpy_s(lad.crsBuffer.data(), lad.crsBuffer.size(), lasCrs.getPrettyWKT().c_str());
		}
		else {
			lad.crsString = "Infer From Files";
			strcpy_s(lad.crsBuffer.data(), lad.crsBuffer.size(), "");
		}
		lad.unitsRadio = unitNameToRadio.at(opt.getLasUnits().name);

		auto& dad = demAdvancedData;
		CoordRef demCrs = opt.getDemCrs();
		if (!demCrs.isEmpty()) {
			dad.crsString = demCrs.getSingleLineWKT();
			strcpy_s(dad.crsBuffer.data(), dad.crsBuffer.size(), demCrs.getPrettyWKT().c_str());
		}
		else {
			dad.crsString = "Infer From Files";
			strcpy_s(dad.crsBuffer.data(), dad.crsBuffer.size(), "");
		}
		dad.unitsRadio = unitNameToRadio.at(opt.getDemUnits().name);

		startOfFrameUserUnits = unitNameToRadio.at(opt.getUserUnits().name);
		userUnitsRadio = unitNameToRadio.at(opt.getUserUnits().name);

		auto& wu = withUnits;
		auto copyDouble = [&](FloatBuffer& b, double x) {
			std::string s = std::to_string(x).c_str();
			s.erase(s.find_last_not_of('0') + 1, std::string::npos);
			s.erase(s.find_last_not_of('.') + 1, std::string::npos);
			strncpy_s(b.data(), b.size(), s.c_str(), b.size());
		};
		copyDouble(wu.canopyBuffer, opt.getCanopyCutoff());
		copyDouble(wu.csmCellsizeBuffer, opt.getCSMCellsize());
		copyDouble(wu.footprintDiamBuffer, opt.getFootprintDiameter());
		copyDouble(wu.maxHtBuffer, opt.getMaxHt());
		copyDouble(wu.minHtBuffer, opt.getMinHt());
		auto& a = opt.getAlign();
		copyDouble(wu.xOriginBuffer, a.xorigin);
		copyDouble(wu.yOriginBuffer, a.yorigin);
		copyDouble(wu.xResBuffer, std::abs(a.xres));
		copyDouble(wu.yResBuffer, std::abs(a.yres));
		
		if (a.xres == a.yres) {
			copyDouble(wu.cellsizeBuffer, std::abs(a.xres));
		}
		else {
			strcpy_s(wu.cellsizeBuffer.data(), wu.cellsizeBuffer.size(), "Diff. X/Y");
		}
		wu.strataBuffers.clear();
		std::vector<double> strata = opt.getStrataBreaks();
		for (size_t i = 0; i < strata.size(); ++i) {
			wu.strataBuffers.emplace_back();
			copyDouble(wu.strataBuffers[i], strata[i]);
		}

		if (!a.crs.isEmpty()) {
			outCRSString = a.crs.getSingleLineWKT();
			strcpy_s(outCRSBuffer.data(), outCRSBuffer.size(), a.crs.getPrettyWKT().c_str());
		}
		else {
			outCRSString = "Same as LAS Files";
			strcpy_s(outCRSBuffer.data(), outCRSBuffer.size(), "");
		}

		smoothRadio = opt.getSmoothWindow();
		filterWithheldCheck = !opt.getUseWithheldFlag();
		firstReturnsCheck = opt.getFirstFlag();
		auto classes = opt.getClassFilter();
		for (int i = 0; i < classesCheck.size(); ++i) {
			if (classes.list.count(i) && classes.isWhiteList) {
				classesCheck[i] = true;
			}
			else if (classes.list.count(i) == 0 && !classes.isWhiteList) {
				classesCheck[i] = true;
			}
			else {
				classesCheck[i] = false;
			}
		}
		updateClassListString(*this);
		fineint = opt.getFineIntFlag();
		advancedpoint = opt.getAdvancedPointFlag();
	}

}