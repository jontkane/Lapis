#ifdef _WIN32
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include<windows.h>
#include<shellapi.h>
#endif


#include"LapisController.hpp"
#include"Options.hpp"
#include"LapisGui.hpp"
#include<sstream>

using namespace lapis;

int unifiedMain(std::vector<std::string> args) {
	if (!args.size()) {
		args.push_back("--help");
	}
	ParseResults parsed = parseOptions(args);
	if (parsed == ParseResults::invalidOpts) {
		std::cout << "Error parsing command line\n";
		return 1;
	}
	else if (parsed == ParseResults::helpPrinted) {
		return 0;
	}
	else if (parsed == ParseResults::guiRequested) {
		renderGui();
		return 0;
	}

	Logger& log = Logger::getLogger();

	LapisController lc;
	try {
		lc.processFullArea();
	}
	catch (std::exception e) {
		log.logError(e.what());
		log.logProgress("Run Aborted");
		return 1;
	}
	catch (...) {
		log.logError("Unknown Error\nRun Aborted");
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
		renderGui();
		return 0;
	}
	std::array<char, 30> oldTitle = std::array<char, 30>();
	auto oldTitle = GetConsoleTitle(oldTitle.data(), oldTitle.size());
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
	
	//semi-ordered to-do list:
	//write a gui

	//add html metadata, in a way that isn't a pain in the ass to extend

	//do an optimization pass
	//	memory usage of sparse histograms
	//	whatever is causing alignment calculation to be really slow--is it possible to reuse coordtransform objects?
	//	profile time spent on various per-point tasks to identify the bottlenecks, and thus where to devote optimization efforts:
	//		reading off the harddrive and decompressing
	//		applying filters and populating the initial data structure
	//		normalizing
	//		copying to skip un-normalized and outlier points
	//		projecting to output CRS
	//		sorting to PointMetricCalculators

	//consider odd parameters--turn off csm creation? turn off wkt cleanup? write in img?

	//check if the occasional multi-cm disagreement between fusion and lapis is because I'm calculating the bilinear interp *wrong* (as opposed to *different*)

	//add checkpointing

	//add smoothing and hole filling to csm

	//add watershed segmentation

	//another optimization pass

	//add automatic unit detection

	//add advanced point metrics

	//add csm metrics

	//add topographic metrics

	//another optimization pass

	//add plot metrics

	//make an installer

	//write a manual

	//one final optimization pass

