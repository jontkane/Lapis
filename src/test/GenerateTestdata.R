#This code generates both the data used in regression testing and the results to test against.
#This script should be run with echo *off*. In R studio, the shortcut for that is ctrl+shift+S

library(terra)
library(lidR)
library(rstudioapi)
library(EBImage)

unlink(file.path(dirname(getSourceEditorContext()$path),"TestData"),recursive=T)


dem = rast(xmin=0,xmax=100,ymin=0,ymax=100,res=1,crs="EPSG:32610+5703")
values(dem) = runif(ncell(dem),100,105)

alignment = rast(xmin=0,xmax=100,ymin=0,ymax=100,res=10,crs="EPSG:32610+5703")

badclasses = c(7,9,18)
goodclasses = 0:20
goodclasses = goodclasses[!(goodclasses %in% badclasses)]

init = function(outputfolder) {
  dir.create(outputfolder,showWarnings=F,recursive=T)
  
  set.seed(0)
  
  X <<- c()
  Y <<- c()
  Z <<- c()
  ReturnNumber <<- c()
  Classification <<- c()
  Withheld_flag <<- c()
  ScanAngle <<- c()
}

addPoints = function(n, x = runif(n,0.5,99.5), y = runif(n,0.5,99.5),
                     z = rep(10,n),returnnumber=rep(1,n),
                     classification = sample(goodclasses,n,T),
                     withheld_flag = rep(F,n),
                     scanangle = runif(n,-10,10)) {
  X <<- c(X,x)
  Y <<- c(Y,y)
  Z <<- c(Z,z+as.numeric(extract(dem,vect(cbind(x,y)),method="bilinear")[,2]))
  ReturnNumber <<- c(ReturnNumber,returnnumber)
  Classification <<- c(Classification,classification)
  Withheld_flag <<- c(Withheld_flag,withheld_flag)
  ScanAngle <<- c(ScanAngle,scanangle)
}

makeLasObject = function(crs) {
  ReturnNumber = as.integer(ReturnNumber)
  Classification = as.integer(Classification)
  data = data.frame(X,Y,Z,ReturnNumber,Classification,Withheld_flag,ScanAngle)
  suppressWarnings(l <- LAS(data,header=list(`Version Major`=1,`Version Minor` = 4,
                                             `X offset`=0,`Y offset`=0,`Z offset`=0,
                                             `X scale factor`=0.01,`Y scale factor`=0.01,`Z scale factor`=0.01,
                                             `File Source ID`=0,
                                             `Global Encoding`=list(`GPS Time Type`=T,`Waveform Data Packets Internal`=F,
                                                                    `Waveform Data Packets External`=F,`Synthetic Return Numbers`=T,
                                                                    `WKT`=T,`Aggregate Model`=F),
                                             `File Creation Day of Year` = 1,
                                             `File Creation Year` = 2023,
                                             `Point Data Format ID` = 6,
                                             `Header Size`=375,
                                             `Project ID - GUID` = "00000000-0000-0000-0000-000000000000")))
  crs(l) = crs
  return(l)
}

makeIni = function(params, outfile) {
  
  ini = ""
  
  addDefaultParam = function(p,v) {
    if (p %in% names(params)) {
      return()
    }
    ini <<- paste0(ini,p,"=",v,"\n")
  }
  
  addDefaultParam("dem-units","unspecified")
  addDefaultParam("dem-algo","raster")
  addDefaultParam("las-units","unspecified")
  addDefaultParam("xres","10")
  addDefaultParam("yres","10")
  addDefaultParam("xorigin","0")
  addDefaultParam("yorigin","0")
  addDefaultParam("csm-cellsize","1")
  addDefaultParam("footprint","0.4")
  addDefaultParam("smooth","3")
  addDefaultParam("minht","-8")
  addDefaultParam("maxht","100")
  addDefaultParam("class","~7,9,18")
  addDefaultParam("max-scan-angle","30")
  addDefaultParam("user-units","meters")
  addDefaultParam("canopy","1.5")
  addDefaultParam("strata","0.5,5.5,10.5,")
  addDefaultParam("min-tao-dist","0")
  addDefaultParam("seg-algo","watershed")
  addDefaultParam("id-algo","highpoint")
  addDefaultParam("topo-scale","500,1000,2000,")
  
  for (n in names(params)) {
    ini = paste0(ini,n,"=",params[[n]],"\n")
  }
  sink(outfile)
  cat(ini)
  sink(NULL)
}

#each test's data is siloed into its own function so it's collapsible in RStudio
makePointMetricAndFilterTest = function() {
  outputfolder = file.path(dirname(getSourceEditorContext()$path),"TestData","PointMetricsTest")
  init(outputfolder)
  writeRaster(dem,file.path(outputfolder,"dem.tif"),overwrite=T)
  
  #Data for testing point metrics and filters.
  #After normalizing and applying filters, every 10m cell should have Z values which are 1 through 50.
  #The values from 0 to 20 are first returns, and the others are second returns
  for (col in 1:10) {
    for (row in 1:10) {
      xcenter = xFromCol(alignment,col)
      ycenter = yFromRow(alignment,row)
      xthis = runif(51,xcenter-4.9,xcenter+4.9)
      ythis = runif(51,ycenter-4.9,ycenter+4.9)
      
      addPoints(51, xthis,ythis,0:50,
                returnnumber=c(rep(1,length(0:20)),rep(2,length(21:50))))
    }
  }
  
  #adding points that should be filtered by a minht of -8 and maxht of 100
  xthis = runif(50,0,100)
  ythis = runif(50,0,100)
  addPoints(50, xthis,ythis,rep(-10,50))
  
  zthis = as.numeric((110 + extract(dem,vect(cbind(xthis,ythis)),method="bilinear"))[,2])
  addPoints(50, xthis,ythis,rep(110,50))
  
  #adding points from filtered classes
  addPoints(50, classification = sample(badclasses, 50, T))
  
  #adding withheld points
  addPoints(50, withheld_flag = rep(T,50))
  
  #adding points that would be filtered by a max scan angle of 30
  addPoints(50, scanangle = c(rep(-40,25),rep(40,25)))
  
  l = makeLasObject("EPSG:32610+5703")
  
  #splitting the las into 10 so the testing can have multiple files to work with
  start = 1
  n = nrow(l@data)/10
  for (i in 1:10) {
    part = l[start:(start+n-1),]
    writeLAS(part,file.path(outputfolder,paste0(i,".laz")))
    start = start + n
  }
  
  params = list(`skip-csm`="",`skip-tao`="",`skip-topo`="",`adv-point`="",las=outputfolder,
                dem=outputfolder,output=paste0(outputfolder,"Output"),name="PointMetricsTest")
  makeIni(params,file.path(outputfolder,"params.ini"))
}

makeCsmAndTaoTest = function() {
  outputfolder = file.path(dirname(getSourceEditorContext()$path),"TestData","CSMTest")
  init(outputfolder)
  writeRaster(dem,file.path(outputfolder,"dem.tif"),overwrite=T)
  
  csmAlign = rast(xmin=0,xmax=100,ymin=0,ymax=100,res=1,crs="EPSG:32610+5703")
  
  #the strategy here is to generate random top-of-canopy, run it through R tools to get an expected output, and then have the C++ test check
  #for equality with the R-generated data
  
  addPoints(10000,z=rnorm(10000,50,10))
  l = makeLasObject("EPSG:32610+5703")
  
  l$X = round(l$X,2)
  l$Y = round(l$Y,2)
  
  start = 1
  n = nrow(l@data)/10
  for (i in 1:10) {
    part = l[start:(start+n-1),]
    writeLAS(part,file.path(outputfolder,paste0(i,".laz")))
    start = start + n
  }
  
  l$Z = round(l$Z-extract(dem,l@data[,c("X","Y")],method="bilinear")[,2],2)
  
  prepareOneTest = function(csm,lapisParams,name) {
    taos = locate_trees(csm,lmf(3,2,"square"))
    taos = vect(taos)
    segcloud = segment_trees(l,lidR::watershed(csm))
    suppressWarnings(segrast <- csmAlign)
    suppressWarnings(v <- values(segrast))
    for (i in 1:nrow(segcloud)) {
      cell = cellFromXY(segrast,segcloud@data[i,1:2])
      v[cell] = segcloud$treeID[i]
    }
    values(segrast) = v
    
    lapisParams[["skip-topo"]] = ""
    lapisParams[["skip-point-metrics"]] = ""
    lapisParams[["output"]] = paste0(outputfolder,"Output")
    lapisParams[["las"]] = outputfolder
    lapisParams[["dem"]] = file.path(outputfolder,"dem.tif")
    lapisParams[["name"]] = name
    lapisParams[["no-csm-fill"]] = ""
    makeIni(lapisParams,file.path(outputfolder,paste0(name,".ini")))
    suppressWarnings(dir.create(file.path(outputfolder,name),recursive=T))
    writeRaster(csm,file.path(outputfolder,name,"csm.tif"),overwrite=T)
    
    writeVector(taos,file.path(outputfolder,name,"taos.shp"),overwrite=T)
    writeRaster(segrast,file.path(outputfolder,name,"segments.tif"),overwrite=T)
  }
  csm = rasterize_canopy(l,1,p2r(0))
  prepareOneTest(csm,list(footprint="0",smooth="1"),"NoSmoothNoFootprint")
  csm = rasterize_canopy(l,1,p2r(0.2222))
  prepareOneTest(csm,list(footprint="0.4444",smooth="1"),"Footprint")
  csm = focal(rasterize_canopy(l,1,p2r(0)),fun="mean",na.rm=T,na.policy="omit")
  prepareOneTest(csm,list(footprint="0",smooth="3"),"Smooth")
  
  
}

makePointMetricAndFilterTest()
makeCsmAndTaoTest()
