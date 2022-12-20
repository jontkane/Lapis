#include"app_pch.hpp"
#include"lapisgui.hpp"
#include"LapisData.hpp"

namespace lapis {

	void renderFullGui()
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

		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			guiTabs();

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
	void guiTabs()
	{
		constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);

		bool isOpen = true;
		ImGui::Begin("Lapis", &isOpen, flags);


		ImGui::BeginTabBar("MainTabs");
		LapisData& d = LapisData::getDataObject();
		LapisGuiObjects& lgo = LapisGuiObjects::getGuiObjects();

		if (!lgo.isRunning()) {
			d.updateUnits();
			d.setPrevUnits(d.outUnits());
			if (ImGui::BeginTabItem("Run")) {
				runTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Data Options")) {
				dataTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Processing Options")) {
				processingOptions();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Product Options")) {
				productOptions();
				ImGui::EndTabItem();
			}
		}
		else {
			if (ImGui::Button("Cancel")) {
				d.setNeedAbortTrue();
				LapisLogger::getLogger().setProgress(RunProgress::canceling);
			}
			displayLog();
		}
		ImGui::EndTabBar();

		ImGui::End();
	}
	void dataTab()
	{
		LapisData& d = LapisData::getDataObject();
		LapisGuiObjects& lgo = LapisGuiObjects::getGuiObjects();

		ImGui::BeginChild("LasButtonsChild", ImVec2(ImGui::GetContentRegionMax().x * 0.5f - 2, 260), true, 0);
		d.renderGui(ParamName::las);
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("DemButtonsChild", ImVec2(ImGui::GetContentRegionMax().x * 0.5f - 2, 260), true, 0);
		d.renderGui(ParamName::dem);
		ImGui::EndChild();
		ImGui::Checkbox("Show Advanced Options", &lgo.showAdvancedDataOptions);
		if (lgo.showAdvancedDataOptions) {
			ImGui::BeginChild("AdvancedLas", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 2, ImGui::GetContentRegionAvail().y), true, 0);
			d.renderGui(ParamName::lasOverride);
			ImGui::EndChild();
			ImGui::SameLine();
			ImGui::BeginChild("AdvancedDem", ImVec2(ImGui::GetContentRegionAvail().x - 2, ImGui::GetContentRegionAvail().y), true, 0);
			d.renderGui(ParamName::demOverride);
			ImGui::EndChild();
		}
	}
	void runTab() {
		LapisData& d = LapisData::getDataObject();
		LapisGuiObjects& lgo = LapisGuiObjects::getGuiObjects();


		d.renderGui(ParamName::name);
		d.renderGui(ParamName::output);
		
		if (ImGui::Button("Start Run")) {
			if (!lgo.isRunning()) {
				lgo.controller.reset(new LapisController);
				auto& log = LapisLogger::getLogger();
				log.reset();
				auto f = [&] {lgo.controller->processFullArea(); };
				lgo.runThread = std::thread(f);
				lgo.runThread.detach();
			}
		}

		static const nfdu8filteritem_t iniFileFilter{ "ini files", "ini" };
		ImGui::SameLine();
		if (ImGui::Button("Import Parameters")) {
			NFD::OpenDialog(lgo.inputIniFile, &iniFileFilter, 1);
		}
		if (lgo.inputIniFile) {
			d.parseIni(lgo.inputIniFile.get());
			lgo.inputIniFile.reset();
		}

		ImGui::SameLine();
		if (ImGui::Button("Export Parameters")) {
			NFD::OpenDialog(lgo.outputIniFile, &iniFileFilter, 1);
		}
		if (lgo.outputIniFile) {
			std::ofstream ofs{ lgo.outputIniFile.get() };
			if (ofs) {
				d.writeOptions(ofs, ParamCategory::data);
				ofs << "\n";
				d.writeOptions(ofs, ParamCategory::computer);
				ofs << "\n";
				d.writeOptions(ofs, ParamCategory::process);
			}
			else {
				LapisLogger::getLogger().logMessage("Unable to write to " + std::string(lgo.outputIniFile.get()));
			}
			lgo.outputIniFile.reset();
		}

		ImGui::SameLine();
		if (ImGui::Button("Check Data for Issues")) {
			lgo.displayDataIssuesWindow = true;
		};
		displayLog();
		dataIssuesWindow();
	}

	void processingOptions()
	{
		LapisData& d = LapisData::getDataObject();

		ImGui::BeginChild("units", getRegionSize(0), true, 0);
		d.renderGui(ParamName::outUnits);
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("alignment", getRegionSize(1), true, 0);
		d.renderGui(ParamName::alignment);
		ImGui::EndChild();

		ImGui::BeginChild("filter", getRegionSize(2), true, 0);
		d.renderGui(ParamName::filters);
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("computer", getRegionSize(3), true, 0);
		d.renderGui(ParamName::computerOptions);
		ImGui::EndChild();
	}

	void productOptions()
	{
		//check boxes for what to compute at the top
		//then tabs for each of those to customize them
		LapisData& d = LapisData::getDataObject();

		ImGui::BeginChild("products", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.5f), true, 0);
		d.renderGui(ParamName::whichProducts);
		ImGui::EndChild();

		ImGui::BeginTabBar("producttabs");

		if (d.doPointMetrics() && ImGui::BeginTabItem("Point Metrics")) {
			d.renderGui(ParamName::pointMetricOptions);
			ImGui::EndTabItem();
		}
		if (d.doCsm() && ImGui::BeginTabItem("Canopy Surface Model")) {
			d.renderGui(ParamName::csmOptions);
			ImGui::EndTabItem();
		}
		if (d.doTaos() && ImGui::BeginTabItem("Tree ID")) {
			d.renderGui(ParamName::taoOptions);
			ImGui::EndTabItem();
		}
		if (d.doTopo() && ImGui::BeginTabItem("Topography")) {
			d.renderGui(ParamName::topoOptions);
			ImGui::EndTabItem();
		}
		if (d.doFineInt() && ImGui::BeginTabItem("Fine-Scale Intensity")) {
			d.renderGui(ParamName::fineIntOptions);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
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

	LapisGuiObjects& LapisGuiObjects::getGuiObjects()
	{
		static LapisGuiObjects lgo;
		return lgo;
	}

	bool LapisGuiObjects::isRunning()
	{
		return controller && controller->isRunning();
	}

	void displayLog() {
		ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar;
		ImGui::BeginChild("log", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), true,
			flags);
		auto& log = LapisLogger::getLogger();
		log.renderGui();
		ImGui::EndChild();
	}

	void dataIssuesWindow() {
		LapisGuiObjects& lgo = LapisGuiObjects::getGuiObjects();
		if (!lgo.displayDataIssuesWindow) {
			return;
		}
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
		ImGui::SetNextWindowSize(ImVec2(600, 400));
		ImGui::SetNextWindowPos(ImVec2(10, 50));
		if (!ImGui::Begin("##dataissueswindow", &lgo.displayDataIssuesWindow, flags)) {
			ImGui::End();
			return;
		}

		static std::unique_ptr<LapisData::DataIssues> dataIssues;

		static bool threadWorking = false;
		auto threadFunc = [&]() {
			dataIssues = std::make_unique<LapisData::DataIssues>(LapisData::getDataObject().checkForDataIssues());
			threadWorking = false;
		};

		std::thread calcIssuesThread;
		if (!threadWorking && !dataIssues) {
			threadWorking = true;
			calcIssuesThread = std::thread(threadFunc);
			calcIssuesThread.detach();
		}
		if (threadWorking) {
			ImGui::Text("Working...");
			ImGui::Text("This may take a few minutes.");
			ImGui::End();
			return;
		}
		int issueCount = 0;
		LapisData::DataIssues& di = *dataIssues;
		ImGuiWindowFlags childflags = ImGuiWindowFlags_HorizontalScrollbar;
		ImGui::BeginChild("##dataissueschild", { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.9f }, false, childflags);
		if (di.failedLas.size() > 0) {
			static bool showFailedLas = false;
			issueCount++;
			ImGui::Text((std::to_string(di.failedLas.size()) + " las/laz files failed to read.").c_str());
			ImGui::SameLine();
			if (!showFailedLas) {
				if (ImGui::Button("Show##failedlas")) {
					showFailedLas = true;
				}
			}
			else {
				if (ImGui::Button("Hide##failedlas")) {
					showFailedLas = false;
				}
				for (size_t i = 0; i < di.failedLas.size(); ++i) {
					ImGui::Text("\t");
					ImGui::SameLine();
					ImGui::Text(di.failedLas[i].c_str());
				}
			}
			ImGui::Text("\n\n");
		}

		if (di.successfulLas.size() == 0) {
			ImGui::Text("No las/laz files found\n\n");
			issueCount++;
		}

		if (di.successfulDem.size() == 0) {
			ImGui::Text("No dem files successfully read\n\n");
			issueCount++;
		}

		if (di.byFileYear.size() > 1) {
			ImGui::Text("Not all las files were created in the same year.\nProcessing multiple acquisitions in one run may cause problems.\nProceed cautiously.\n\n");
			issueCount++;
		}

		if (di.byCrs.size() > 1) {
			ImGui::Text("The las files contain multiple\nCoordinate Reference Systems.\nProcessing multiple acquisitions in one run may cause problems.\nProceed cautiously.\n\n");
			issueCount++;
		}

		int percentInDem = (int)((double)di.cellsInDem / (double)di.cellsInLas * 100.);
		if (di.cellsInDem > 0 && percentInDem < 100) {
			issueCount++;
			std::string message = "Only " + std::to_string(percentInDem) + "% of las bounding boxes covered by dems.\n"
				"If this value is close to 100%, it may be an artifact and not an issue.\n\n";
			ImGui::TextUnformatted(message.c_str());
		}

		if (di.byCrs.contains(CoordRef())) {
			ImGui::Text("Some las files don't have their CRS specified in their header.\n\n");
			issueCount++;
		}

		for (auto& v : di.byCrs) {
			if (v.first.getZUnits().status == unitStatus::inferredFromCRS) {
				std::string msg = v.first.getShortName() + " in las file does not specify Z units.\nGuessing " + v.first.getZUnits().name + "\n\n";
				ImGui::Text(msg.c_str());
				issueCount++;
			}
		}

		for (auto& v : di.demByCrs) {
			if (v.first.getZUnits().status == unitStatus::inferredFromCRS) {
				std::string msg = v.first.getShortName() + " in DEM does not specify Z units.\nGuessing " + v.first.getZUnits().name + "\n\n";
				ImGui::Text(msg.c_str());
				issueCount++;
			}
		}

		if (di.totalArea > 0 && di.overlapArea > 0.01 * di.totalArea) {
			int percent = (int)(di.overlapArea / di.totalArea * 100.);
			std::string msg = std::to_string(percent) + "% of area is covered by the extent of multiple las files.\n"
				"If points are duplicated, that can produce bad metrics.\n"
				"If this value is close to 0%, it may be an artifact and not an issue.\n\n";
			ImGui::TextUnformatted(msg.c_str());
			issueCount++;
		}

		if (di.pointsInSample) {
			int percentAfterFilters = (int)((double)di.pointsAfterFilters / (double)di.pointsInSample * 100.);
			int percentAfterDem = (int)((double)di.pointsAfterDem / (double)di.pointsInSample * 100.);

			if (percentAfterDem < 100) {
				ImGui::Text("In a sample las file,\n");
				std::string demMsg = std::to_string(percentAfterDem) + "% of points passed filters";
				ImGui::TextUnformatted(demMsg.c_str());
				ImGui::Text("If this number is unexpectedly low, it may be an issue with your DEMs");
				std::string filterMsg = "(" + std::to_string(percentAfterFilters) + "% of points passed filters ignoring the DEM)";
				ImGui::TextUnformatted(filterMsg.c_str());
			}
		}

		//TODO:
		/*feet outside of us
		improbable units based on coarse dem*/

		if (issueCount == 0) {
			ImGui::Text("No issues detected.");
		}
		ImGui::EndChild();

		if (ImGui::Button("Okay")) {
			lgo.displayDataIssuesWindow = false;
			dataIssues.reset();
		}
		ImGui::End();
	}

}