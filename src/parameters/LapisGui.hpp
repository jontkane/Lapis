#pragma once
#ifndef LP_LAPISGUI_H
#define LP_LAPISGUI_H

#include"param_pch.hpp"
#include"RunParameters.hpp"
#include"AllParameters.hpp"
#include"..\utils\LapisFonts.hpp"

namespace lapis {

	template<class RUNNERTYPE>
	class LapisGui {
	public:
		static LapisGui<RUNNERTYPE>& singleton();
		void renderFullGui();

		template<class PARAMETER>
		void addModParameter(const std::string& name);

	private:

		LapisGui();
		LapisGui(const LapisGui&) = delete;
		LapisGui(LapisGui&&) = delete;

		void _mainTabs();
		void _runTab();
		void _generalOptionsTab();
		void _productsTab();
		void _renderLogger();
		void _dataIssuesWindow();
		void _modsTab();

		std::unique_ptr<RUNNERTYPE> _runner;
		std::thread _runThread;

		bool _displayDataIssuesWindow = false;

		struct NameAndRenderer {
			std::string name;
			std::function<void(const std::string&)> renderer;
		};
		std::vector<NameAndRenderer> _mods;
	};

	template<class RUNNERTYPE>
	inline LapisGui<RUNNERTYPE>& LapisGui<RUNNERTYPE>::singleton()
	{
		static LapisGui<RUNNERTYPE> lg;
		return lg;
	}

	template<class RUNNERTYPE>
	inline void LapisGui<RUNNERTYPE>::renderFullGui()
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
		GLFWwindow* window = glfwCreateWindow(960, 900, "Lapis", nullptr, nullptr);
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
		
		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_TabActive] = ImVec4(0.6f, 0.4f, 1.0f, 1.0f);

		LapisFonts::initFonts();

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init(glsl_version);


		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::PushFont(LapisFonts::getRegularFont());

			_mainTabs();

			ImGui::PopFont();

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

	template<class RUNNERTYPE>
	template<class PARAMETER>
	inline void LapisGui<RUNNERTYPE>::addModParameter(const std::string& name)
	{
		auto renderModTab = [](const std::string& name) {
			if (ImGui::BeginTabItem(name.c_str())) {
				RunParameters::singleton().renderGui<PARAMETER>();
				ImGui::EndTabItem();
			}
		};
		_mods.emplace_back(name, std::function(renderModTab));
	}

	template<class RUNNERTYPE>
	inline LapisGui<RUNNERTYPE>::LapisGui() : _runner(new RUNNERTYPE())
	{
	}

	template<class RUNNERTYPE>
	inline void LapisGui<RUNNERTYPE>::_mainTabs()
	{
		constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);

		bool isOpen = true;
		ImGui::Begin("Lapis", &isOpen, flags);


		ImGui::BeginTabBar("MainTabs");
		RunParameters& rp = RunParameters::singleton();

		if (!_runner->isRunning()) {
			rp.updateUnits();
			rp.setPrevUnits(rp.outUnits());
			if (ImGui::BeginTabItem("Run")) {
				_runTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Las Files")) {
				rp.renderGui<LasFileParameter>();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Ground Model")) {
				rp.renderGui<DemParameter>();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Processing Options")) {
				_generalOptionsTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Product Options")) {
				_productsTab();
				ImGui::EndTabItem();
			}
			if (_mods.size() && ImGui::BeginTabItem("Mods")) {
				_modsTab();
				ImGui::EndTabItem();
			}
		}
		else {
			if (ImGui::Button("Cancel")) {
				_runner->sendAbortSignal();
				LapisLogger::getLogger().setProgress("Cancelling");
			}
			_renderLogger();
		}
		ImGui::EndTabBar();

		ImGui::End();
	}

	template<class RUNNERTYPE>
	inline void LapisGui<RUNNERTYPE>::_runTab()
	{
		RunParameters& rp = RunParameters::singleton();


		rp.renderGui<NameParameter>();
		rp.renderGui<OutputParameter>();

		if (ImGui::Button("Start Run")) {
			if (!_runner->isRunning()) {
				_runner.reset(new RUNNERTYPE()); //just in case
				auto& log = LapisLogger::getLogger();
				log.reset();
				auto f = [&] {_runner->processFullArea(); };
				_runThread = std::thread(f);
				_runThread.detach();
			}
		}

		static const nfdu8filteritem_t iniFileFilter{ "ini files", "ini" };
		ImGui::SameLine();
		static NFD::UniquePathU8 inputIniFile;
		if (ImGui::Button("Load Parameters")) {
			NFD::OpenDialog(inputIniFile, &iniFileFilter, 1);
		}
		if (inputIniFile) {
			rp.parseIni(inputIniFile.get());
			inputIniFile.reset();
		}

		auto writeOptions = [&](const std::string& x) {
			
		};

		static NFD::UniquePathU8 outputIniFile;
		ImGui::SameLine();
		if (ImGui::Button("Save Parameters")) {
			NFD::SaveDialog(outputIniFile, &iniFileFilter, 1);
		}
		if (outputIniFile) {
			std::ofstream ofs{ outputIniFile.get()};
			if (ofs) {
				rp.writeOptions(ofs, ParamCategory::data);
				ofs << "\n";
				rp.writeOptions(ofs, ParamCategory::computer);
				ofs << "\n";
				rp.writeOptions(ofs, ParamCategory::process);
			}
			else {
				LapisLogger::getLogger().logWarning("Unable to write options to " + std::string(outputIniFile.get()));
			}
			outputIniFile.reset();
		}

		ImGui::SameLine();
		if (ImGui::Button("Save Parameters as Default")) {
			std::filesystem::path defaultini = executableFilePath();
			defaultini = defaultini.parent_path() / "lapisdefault.ini";
			std::ofstream ofs{ defaultini.string() };
			if (ofs) {
				rp.writeOptions(ofs, ParamCategory::computer);
				ofs << "\n";
				rp.writeOptions(ofs, ParamCategory::process);
			}
			else {
				LapisLogger::getLogger().logWarning("Unable to write options to " + defaultini.string());
			}
			LapisLogger::getLogger().logMessage("Current parameters written to " + defaultini.string());
		}

		//ImGui::SameLine();
		//if (ImGui::Button("Check Data for Issues")) {
		//	_displayDataIssuesWindow = true;
		//};
		_renderLogger();
		//_dataIssuesWindow();
	}

	template<class RUNNERTYPE>
	inline void LapisGui<RUNNERTYPE>::_generalOptionsTab()
	{
		RunParameters& rp = RunParameters::singleton();

		ImGui::BeginChild("##optionstab", ImGui::GetContentRegionAvail(), false);
		static constexpr int columns = 2;
		static constexpr int rows = 2;
		static ImVec2 subAreaSize = { ImGui::GetContentRegionMax().x / columns - 5, ImGui::GetContentRegionMax().y / rows - 5 };

		ImGui::BeginChild("units", subAreaSize, true, 0);
		rp.renderGui<OutUnitParameter>();
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("alignment", subAreaSize, true, 0);
		rp.renderGui<AlignmentParameter>();
		ImGui::EndChild();

		ImGui::BeginChild("filter", subAreaSize, true, 0);
		rp.renderGui<FilterParameter>();
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("computer", subAreaSize, true, 0);
		rp.renderGui<ComputerParameter>();
		ImGui::EndChild();

		ImGui::EndChild();
	}

	template<class RUNNERTYPE>
	inline void LapisGui<RUNNERTYPE>::_productsTab()
	{
		//check boxes for what to compute at the top
		//then tabs for each of those to customize them
		RunParameters& rp = RunParameters::singleton();

		ImGui::BeginChild("products", ImVec2(ImGui::GetContentRegionAvail().x, 235), true, 0);
		rp.renderGui<WhichProductsParameter>();
		ImGui::EndChild();

		ImGui::BeginTabBar("producttabs");

		if (rp.doPointMetrics() && ImGui::BeginTabItem("Point Metrics")) {
			rp.renderGui<PointMetricParameter>();
			ImGui::EndTabItem();
		}
		if (rp.doCsm() && ImGui::BeginTabItem("Canopy Surface Model")) {
			rp.renderGui<CsmParameter>();
			ImGui::EndTabItem();
		}
		if (rp.doTaos() && ImGui::BeginTabItem("Tree ID")) {
			rp.renderGui<TaoParameter>();
			ImGui::EndTabItem();
		}
		if (rp.doTopo() && ImGui::BeginTabItem("Topography")) {
			rp.renderGui<TopoParameter>();
			ImGui::EndTabItem();
		}
		if (rp.doFineInt() && ImGui::BeginTabItem("Fine-Scale Intensity")) {
			rp.renderGui<FineIntParameter>();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	template<class RUNNERTYPE>
	inline void LapisGui<RUNNERTYPE>::_renderLogger()
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar;
		ImGui::BeginChild("log", ImGui::GetContentRegionAvail(), true, flags);
		auto& log = LapisLogger::getLogger();
		log.renderGui();
		ImGui::EndChild();
	}

	template<class RUNNERTYPE>
	inline void LapisGui<RUNNERTYPE>::_dataIssuesWindow()
	{

		if (!_displayDataIssuesWindow) {
			return;
		}
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
		ImGui::SetNextWindowSize(ImVec2(600, 400));
		ImGui::SetNextWindowPos(ImVec2(10, 50));
		if (!ImGui::Begin("##dataissueswindow", &_displayDataIssuesWindow, flags)) {
			ImGui::End();
			return;
		}

		//This code is in desperate need of a refactor, but I'm not ready to do that yet. I don't want to delete the implementation, so instead I'm just commenting it out for now.
		/*static std::unique_ptr<LapisData::DataIssues> dataIssues;

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

		if (issueCount == 0) {
			ImGui::Text("No issues detected.");
		}
		ImGui::EndChild();
		*/
		if (ImGui::Button("Okay")) {
			_displayDataIssuesWindow = false;
			//dataIssues.reset();
		}
		ImGui::End();
	}

	template<class RUNNERTYPE>
	inline void LapisGui<RUNNERTYPE>::_modsTab()
	{
		ImGui::BeginChild("##mods", ImGui::GetContentRegionAvail());
		ImGui::BeginTabBar("##modtabs");
		for (size_t i = 0; i < _mods.size(); ++i) {
			_mods[i].renderer(_mods[i].name);
		}
		ImGui::EndTabBar();
		ImGui::EndChild();
	}

	//forcing the compiler to check that this template is fine without having to go to other static libraries.
	class EmptyRunner {
	public:
		void processFullArea() {}
		bool isRunning() { return false; }
		void sendAbortSignal() {}
	};
	template class LapisGui<EmptyRunner>;
}

#endif