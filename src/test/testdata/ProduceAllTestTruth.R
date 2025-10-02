library(terra)
library(lidR)
library(yaml)

inputfiles = read_yaml(file.path(outputfolder,"tif_laz_associations.yaml"))
tests = read_yaml(file.path(inputfolder,"AlgorithmParamsTestValues.yaml"))

doOneAlgorithm = function(name, yamlfragment) {
  if (length(yamlfragment) == 0) {
    do.call(paste0("do", name), list(yamlfragment))
    return()
  }
  
  combos = expand.grid(yamlfragment, stringsAsFactors = F)
  
  for (i in 1:nrow(combos)) {
    l = list()
    thisparams = as.list(combos[i, ])
    l[["yamlfragment"]] = thisparams
    for (j in 1:length(inputfiles)) {
      l[["input"]] = inputfiles[[j]]
      do.call(paste0("do", name), l)
    }
  }
}

getFullFilename = function(algoname, inputname, yamlfragment, extension) {
  fullname = paste0(algoname,"_",inputname)
  for (i in order(names(yamlfragment))) {
    fullname = paste0(fullname,"_",yamlfragment[[i]])
  }
  fullname = paste0(fullname,".",extension)
  return(file.path(outputfolder,fullname))
}

doAlreadyNormalized = function(yamlfragment, input) {
  #this algorithm is so simple that it doesn't have corresponding R code; the tests will be done entirely in C++
}

doVendorRaster = function(yamlfragment, input) {
  minht = yamlfragment$MinHeight[[1]]
  maxht = yamlfragment$MaxHeight[[1]]
  l = readLAS(input$laz)
  r = lapply(input$tif,rast)
  for (i in 1:length(r)) {
    if (crs(r[[i]])!="") {
      if (i == 1) {
        r[[i]] = project(r[[i]],crs(l)@projargs,method="bilinear")
      } else{
        r[[i]] = project(r[[i]],r[[1]],align_only=T)
      }
    }
  }
  if (length(r)>1) {
    r = do.call(mosaic,r)
  } else {
    r = r[[1]]
  }
  
  zdem = extract(r,l@data[,c("X","Y")],method="bilinear")[,2]
  
  use = !is.na(zdem)
  l = l[use,]
  zdem = zdem[use]
  
  l$Z = l$Z-zdem
  l = l[l$Z>=minht,]
  l = l[l$Z<=maxht,]
  writeLAS(l, getFullFilename("VendorRaster",input$shortname,yamlfragment,"laz"))
}

doMaxPoint = function(yamlfragment, input) {
  normalized = tests$Pipeline$Normalization
  normalized = getFullFilename(names(normalized),input$shortname,normalized[[1]],"laz")
  l = readLAS(normalized)
  r = rasterize_canopy(l,res=yamlfragment$Resolution[[1]],algorithm=p2r(subcircle=yamlfragment$FootprintRadius[[1]]))
  writeRaster(r, getFullFilename("MaxPoint",input$shortname, yamlfragment,"tif"))
}

doDoNothingCsm = function(yamlfragment, input) {
  #this algorithm represents not post-processing the csm
  #as such, the C++ test can just read the existing csm from the pipeline
}
doFillCsm = function(yamlfragment, input) {
  #lidR and lapis do not have particularly comparable filling algorithms
  #as such, this will have to be tested from first principles, not from comparison to lidR
}
doSmoothAndFill = function(yamlfragment, input) {}
doSmoothCsm = function(yamlfragment, input) {}
doHighPoints = function(yamlfragment, input) {}
doMcGaugheySegment = function(yamlfragment, input) {}
doWatershedSegment = function(yamlfragment, input) {}

for (i in 1:length(tests$Tests)) {
  doOneAlgorithm(names(tests$Tests)[[i]], tests$Tests[[i]])
}