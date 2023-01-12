#pragma once
#ifndef LP_LAPIS_H
#define LP_LAPIS_H

#ifdef _WIN32
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include<windows.h>
#include<shellapi.h>
#endif


#include"run/LapisController.hpp"
#include"parameters/LapisGui.hpp"
#include"parameters/RunParameters.hpp"
#include"run/AllHandlers.hpp"

namespace lapis {


	void logGDALErrors(CPLErr eErrClass, CPLErrorNum nError, const char* pszErrorMsg) {
		LapisLogger::getLogger().logMessage(pszErrorMsg);
	}

	void logProjErrors(void* v, int i, const char* c) {
		LapisLogger::getLogger().logMessage(c);
	}

	template<class PARAMETER>
	void addModParameterToGui(const std::string& name) {
		LapisGui<LapisController>::singleton().addModParameter<PARAMETER>(name);
	}

	template<class HANDLER>
	void modProductBehavior(HANDLER* newBehavior) {
		LapisController::replaceHandlerWithMod<HANDLER>(newBehavior);
	}

	void setProjDataDirectory(char* exeName) {
		std::string parent = std::filesystem::path(exeName).parent_path().string();
		char* cStr = new char[parent.size() + 1];
		strncpy_s(cStr,parent.size() + 1, parent.c_str(), parent.size() + 1);
		proj_context_set_search_paths(nullptr, 1, &cStr);

		delete[] cStr;
	}

	int lapisUnifiedMain(std::vector<std::string> args) {
		if (!args.size()) {
			args.push_back("--help");
		}

#ifndef NDEBUG
		CPLSetErrorHandler(&logGDALErrors);
		proj_log_func(ProjContextByThread::get(), nullptr, &logProjErrors);
#else
		CPLSetErrorHandler(silenceGDALErrors);
		proj_log_level(ProjContextByThread::get(), PJ_LOG_NONE);
#endif

		RunParameters& rp = RunParameters::singleton();
		using pr = RunParameters::ParseResults;
		pr parsed = rp.parseArgs(args);
		rp.importBoostAndUpdateUnits();
		if (parsed == pr::invalidOpts) {
			std::cout << "Error parsing command line\n";
			return 1;
		}
		else if (parsed == pr::helpPrinted) {
			return 0;
		}
		else if (parsed == pr::guiRequested) {
			LapisGui<LapisController>::singleton().renderFullGui();
			return 0;
		}

		LapisController lc;
		try {
			lc.processFullArea();
		}
		catch (std::exception e) {
			std::cout << e.what() << "\n";
			std::cout << "Run Aborted\n";
			return 1;
		}
		catch (...) {
			std::cout << "Unknown Error\nRun Aborted\n";
			return 1;
		}

		return 0;
	}

	int lapisMain(int argc, char* argv[]) {
		setProjDataDirectory(argv[0]);
		std::vector<std::string> args;
		for (int i = 1; i < argc; ++i) { //starting from 1 to skip the exe location, to match the WinMain entry
			args.push_back(std::string(argv[i]));
		}
		return lapisUnifiedMain(args);
	}

#ifdef _WIN32
	int APIENTRY lapisWinMain(
		_In_ HINSTANCE hInstance,
		_In_opt_ HINSTANCE hPrevInstance,
		_In_ LPSTR lpCmdLine,
		_In_ int nShowCmd) {

		char exePath[255];
		GetModuleFileNameA(nullptr, exePath, 255);
		setProjDataDirectory(exePath);

		bool attachResult = AttachConsole(ATTACH_PARENT_PROCESS);
		if (!attachResult) { //there is no parent console. Just open the GUI
			LapisGui<LapisController>::singleton().renderFullGui();
			return 0;
		}
		std::array<char, 30> oldTitle = std::array<char, 30>();
		GetConsoleTitle(oldTitle.data(), (DWORD)oldTitle.size());
		SetConsoleTitle("Lapis");

		FILE* newPtr = nullptr;
		freopen_s(&newPtr, "CON", "w", stdout);

		std::vector<std::string> args = boost::program_options::split_winmain(lpCmdLine);
		int result = lapisUnifiedMain(args);
		SetConsoleTitle(oldTitle.data());
		FreeConsole();
		return result;
	}
#endif
}

#endif