#include"app_pch.hpp"
#include"lapisgui.hpp"
#include<locale>
#include<codecvt>
#include"gis/alignment.hpp"
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
			d.importBoostAndUpdateUnits();
			d.setPrevUnits(d.getCurrentUnits());
			if (ImGui::BeginTabItem("Run")) {
				runTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Data Options")) {
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
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Computer Options")) {
				d.renderGui(ParamName::computerOptions);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Processing Options")) {
				processingOptions();
				ImGui::EndTabItem();
			}
		}
		else {
			displayLog();
		}
		ImGui::EndTabBar();

		ImGui::End();
	}
	void runTab() {
		LapisData& d = LapisData::getDataObject();
		LapisGuiObjects& lgo = LapisGuiObjects::getGuiObjects();


		d.renderGui(ParamName::name);
		d.renderGui(ParamName::output);
		
		if (ImGui::Button("Start Run")) {
			if (!lgo.isRunning()) {
				lgo.controller.reset(new LapisController);
				lgo.vlog.clear();
				auto f = [&] {lgo.controller->processFullArea(); };
				lgo.runThread = std::thread(f);
				lgo.runThread.detach();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Import Parameters")) {
			std::unique_ptr<nfdu8filteritem_t> iniFileFilter = std::make_unique<nfdu8filteritem_t>("ini files", "ini");
			NFD::OpenDialog(lgo.inifile, iniFileFilter.get(), 1);
		}

		if (lgo.inifile) {
			d.parseIni(lgo.inifile.get());
			lgo.inifile.reset();
		}

		ImGui::SameLine();
		if (ImGui::Button("Check Parameters for Issues")) {}
		ImGui::SameLine();
		if (ImGui::Button("Check Data for Issues")) {};
		displayLog();

	}

	void processingOptions()
	{
		LapisData& d = LapisData::getDataObject();
		LapisGuiObjects& lgo = LapisGuiObjects::getGuiObjects();

		ImGui::BeginChild("units", getRegionSize(0), true, 0);
		d.renderGui(ParamName::outUnits);
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("alignment", getRegionSize(1), true, 0);
		d.renderGui(ParamName::alignment);
		ImGui::EndChild();

		ImGui::BeginChild("csm", getRegionSize(2), true, 0);
		d.renderGui(ParamName::csmOptions);
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("metric", getRegionSize(3), true, 0);
		d.renderGui(ParamName::metricOptions);
		ImGui::EndChild();

		ImGui::BeginChild("filter", getRegionSize(4), true, 0);
		d.renderGui(ParamName::filterOptions);
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("optional", getRegionSize(5), true, 0);
		d.renderGui(ParamName::optionalMetrics);
		ImGui::EndChild();
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
		auto& lgo = LapisGuiObjects::getGuiObjects();
		for (size_t i = 0; i < lgo.vlog.log.size(); ++i) {
			ImGui::Text(lgo.vlog.log[i].c_str());
		}
		ImGui::EndChild();
	}

}