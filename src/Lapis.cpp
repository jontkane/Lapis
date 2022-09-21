#include"LapisController.hpp"
#include"Options.hpp"
#include"LapisGui.hpp"

using namespace lapis;

int main(int argc, char* argv[])
{
	if (argc < 2) {
		renderingBoilerplate();
	}
	else {
		ParseResults parsed = parseOptions(argc, argv);
		if (parsed == ParseResults::invalidOpts) {
			std::cout << "Error parsing command line\n";
			exit(-1);
		}
		else if (parsed == ParseResults::helpPrinted) {
			exit(-1);
		}

		Logger& log = Logger::getLogger();

		LapisController lc;
		try {
			lc.processFullArea();
			log.logProgress("Done!");
		}
		catch (std::exception e) {
			log.logError(e.what());
			log.logProgress("Run Aborted");
		}
		catch (...) {
			log.logError("Unknown Error\nRun Aborted");
		}
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
}
