#include"lapis_pch.hpp"
#include"LapisController.hpp"
#include"Options.hpp"

using namespace lapis;

namespace fs = std::filesystem;
namespace chr = std::chrono;

int main(int argc, char* argv[])
{
	Logger log;
	auto parsed = parseOptions(argc, argv, log);
	if (std::holds_alternative<std::exception>(parsed)) {
		std::cout << "Error parsing command line\n";
		exit(-1);
	}
	FullOptions& opt = std::get<FullOptions>(parsed);

	LapisController lc{ opt };
	lc.processFullArea();

	log.logProgress("Done!");

	//immediate next task:
	//test usableparameters
	
	//semi-ordered to-do list:
	
	//restructure lapiscontroller to allow for unit testing
	//write unit testing for lapiscontroller elements:
	//	initialization from options (esp. checking outalign, CRSes, and units)
	//	point distribution into raster cells
	//	point metric calculation
	//	csm calculation

	//add parameter output

	//write a gui

	//add html metadata, in a way that isn't a pain in the ass to extend

	//do an optimization pass

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
}
