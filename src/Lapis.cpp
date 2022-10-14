#ifdef _WIN32
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include<windows.h>
#include<shellapi.h>
#endif


#include"LapisController.hpp"
#include"LapisGui.hpp"
#include"LapisData.hpp"
#include<sstream>

using namespace lapis;

int unifiedMain(std::vector<std::string> args) {
	if (!args.size()) {
		args.push_back("--help");
	}
	auto& d = LapisData::getDataObject();
	using pr = LapisData::ParseResults;
	pr parsed = d.parseArgs(args);
	d.importBoostAndUpdateUnits();
	if (parsed == pr::invalidOpts) {
		std::cout << "Error parsing command line\n";
		return 1;
	}
	else if (parsed == pr::helpPrinted) {
		return 0;
	}
	else if (parsed == pr::guiRequested) {
		renderFullGui();
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

#ifdef _WIN32
int APIENTRY WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd
) {
	bool attachResult = AttachConsole(ATTACH_PARENT_PROCESS);
	if (!attachResult) { //there is no parent console. Just open the GUI
		renderFullGui();
		return 0;
	}
	std::array<char, 30> oldTitle = std::array<char, 30>();
	GetConsoleTitle(oldTitle.data(), (DWORD)oldTitle.size());
	SetConsoleTitle("Lapis");

	FILE* newPtr = nullptr;
	freopen_s(&newPtr, "CON", "w", stdout);

	std::vector<std::string> args = boost::program_options::split_winmain(lpCmdLine);
	int result = unifiedMain(args);
	SetConsoleTitle(oldTitle.data());
	FreeConsole();
	return result;
}
#endif

int main(int argc, char* argv[])
{
	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) { //starting from 1 to skip the exe location, to match the WinMain entry
		args.push_back(std::string(argv[i]));
	}
	return unifiedMain(args);
}

