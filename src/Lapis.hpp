#pragma once
#ifndef LP_LAPIS_H
#define LP_LAPIS_H

#include"utils/LapisOSSpecific.hpp"
#include"run/LapisController.hpp"
#include"parameters/LapisGui.hpp"
#include"parameters/RunParameters.hpp"
#include"run/AllHandlers.hpp"


namespace lapis {

	void logGDALErrors(CPLErr eErrClass, CPLErrorNum nError, const char* pszErrorMsg) {
		LapisLogger::getLogger().logWarning(pszErrorMsg);
	}

	void logProjErrors(void* v, int i, const char* c) {
		LapisLogger::getLogger().logWarning(c);
	}

	template<class PARAMETER>
	void addModParameterToGui(const std::string& name) {
		LapisGui<LapisController>::singleton().addModParameter<PARAMETER>(name);
	}

	template<class HANDLER>
	void modProductBehavior(HANDLER* newBehavior) {
		LapisController::replaceHandlerWithMod<HANDLER>(newBehavior);
	}

	void setProjDataDirectory(const char* exeName) {
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
		setProjDataDirectory(executableFilePath().c_str());

		RunParameters& rp = RunParameters::singleton();
		rp.resetObject();
		using pr = RunParameters::ParseResults;
		pr parsed = rp.parseArgs(args);
		rp.importBoostAndUpdateUnits();
		if (parsed == pr::invalidOpts) {
			lapisCout << "Error parsing command line\n";
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
			if (!lc.processFullArea()) {
				lapisCout << "Run Failed\n";
				return 1;
			}
		}
		catch (std::exception e) {
			lapisCerr << e.what() << "\n";
			lapisCout << "Run Aborted\n";
			return 1;
		}
		catch (...) {
			lapisCerr << "Unknown Error\n";
			lapisCout << "Run Aborted\n";
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
	void pressEnter() {
		INPUT ip{};
		// Set up a generic keyboard event.
		ip.type = INPUT_KEYBOARD;
		ip.ki.wScan = 0; // hardware scan code for key
		ip.ki.time = 0;
		ip.ki.dwExtraInfo = 0;

		// Send the "Enter" key
		ip.ki.wVk = 0x0D; // virtual-key code for the "Enter" key
		ip.ki.dwFlags = 0; // 0 for key press
		SendInput(1, &ip, sizeof(INPUT));

		// Release the "Enter" key
		ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
		SendInput(1, &ip, sizeof(INPUT));
	}


	int APIENTRY lapisWinMain(
		_In_ HINSTANCE hInstance,
		_In_opt_ HINSTANCE hPrevInstance,
		_In_ LPSTR lpCmdLine,
		_In_ int nShowCmd) {

		bool commandLine = AttachConsole(ATTACH_PARENT_PROCESS);
		std::array<char, 30> oldTitle = std::array<char, 30>();
		if (commandLine) {
			//lapis was run from the command line
			GetConsoleTitle(oldTitle.data(), (DWORD)oldTitle.size());
			SetConsoleTitle("Lapis");

			lapisCout = LapisWindowsWriter(&std::cout, GetStdHandle(STD_OUTPUT_HANDLE));
			lapisCerr = LapisWindowsWriter(&std::cerr, GetStdHandle(STD_ERROR_HANDLE));
		}

		std::vector<std::string> args = boost::program_options::split_winmain(lpCmdLine);
		if (!commandLine) {
			args.push_back("--gui");
		}
		int result = lapisUnifiedMain(args);
		if (commandLine) {
			SetConsoleTitle(oldTitle.data());
			pressEnter();
			FreeConsole();
		}

		return result;
	}
#endif
}

#endif