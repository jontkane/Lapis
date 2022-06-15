#include"LapisController.hpp"
#include"Options.hpp"

using namespace lapis;

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
	
	
	//semi-ordered to-do list:
	//write the tile layout as a polygon
	//add hooks for mods
	//configure smooth window

	//add tests for CSM merging

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
