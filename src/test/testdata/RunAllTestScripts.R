#workflow:
#GenerateSyntheticData.R
# This will produce synthetic data, and produce a yaml of all plausible laz/tif pairs (including real data)
# might need a rename, since it does two things and the filename only suggests one
#GenerateAllCombinatoricTests.R
# produces a yaml file which lists all the tests to be run
# the input is two yaml files: the one produced by the previous script,
# and one hand-generated, which lists the classes being tested, which parameters each class cares about, and the values to test for each parameter
# the output should have all combinatoric possibilities
#ProduceallTestTruth.R
# this script runs lidR functions which are (in theory) equivalent to my code, using the parameters specified in the output of the previous script
# the C++ tests will then check for approximate equality with these outputs, so lidR is the 'truth'


#algorithms to test:
# AlreadyNormalized --equivalent to just doing nothing
# DoNothingCsm --equivalent to just doing nothing
# FillCsm --this one doesn't have a script lidR equivalent, so I may have no choice but to just test invariates and hope for the best
# HighPoints --equivalent to lmf (local maxima filter)
# MaxPoint --equivalent to p2r (point to raster)
# McGaugheySegment --not in lidR; may have to test against actual treeseg. annoying
# SmoothAndFill --same problems as FillCsm
# SmoothCsm --equivalent to a focal() with mean
# VendorRaster --equivalent to subtracting a raster from a las in lidR
# WatershedSegment --watershed() is in lidR, but it segments the pointcloud, not the csm. I will have to, annoyingly, convert between them

if (!interactive()) {
	args = commandArgs(trailingOnly = T)
	inputfolder = args[1] #this should contain the real (non-synthetic) test data, as well as the input yaml file
	outputfolder = args[2] #this is the desired location for the synthetic data, the produced yaml files, and the lidR and terra 'truth' outputs
} else {
	inputfolder = readline("Input folder: ")
	outputfolder = readline("Output folder: ")
}

unlink(outputfolder, recursive = T)
dir.create(outputfolder, recursive = T)

if (!require("lidR")) {
  install.packages("lidR")
  library("lidR")
}
if (!require("terra")) {
  install.packages("terra")
  library("terra")
}
if (!require("yaml")) {
  install.packages("yaml")
  library("yaml")
}

source(file.path(inputfolder, "GenerateSyntheticData.R"), echo=F)
source(file.path(inputfolder, "ProduceAllTestTruth.R"), echo=F)