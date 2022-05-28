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

	
	//TODO for alpha:
	//write ini

	//TODO for beta:
	//GUI
	//make raster IO understand full hard drives
	//Enable checkpointing
	//double check my withheld implementation with las 1.0 files
	//create a test suite that goes through the various combinations of full runs
	//make sure you don't try to read points with too many user-defined bytes
	//test the various class flags in both point10 and point14
	//investigate better algorithms for estimating quantiles from a histogram
	//write metadata html
	//do an optimization pass--especially alignment creation and CSM merging
	//add a check forbidding lat/lon output

	//TODO for 1.0:
	//the rest of the owl
}
