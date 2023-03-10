#This code generates both the data used in regression testing and the results to test against.
#This script should be run with echo *off*. In R studio, the shortcut for that is ctrl+shift+S

library(terra)
library(lidR)
library(rstudioapi)

outputfolder = file.path(dirname(getSourceEditorContext()$path),"TestData","PointMetricsTest")
dir.create(outputfolder,showWarnings=F,recursive=T)

set.seed(0)

dem = rast(xmin=0,xmax=100,ymin=0,ymax=100,res=1,crs="EPSG:32610+5703")
values(dem) = runif(ncell(dem),100,105)

#lapis' implementation of bilinear interpolation is slightly different from terra's
# lapisBilinear = function(r,x,y) {
#   colToLeft = floor(x-xmin(r)-xres(r)/2)/xres(r)
#   rowAbove = floor(ymax(r)-y-yres(r)/2)/yres(r)
#   
#   xToLeft = xmin(r) + xres(r) * colToLeft + xres(r)/2
#   xToRight = xToLeft + xres(r)
#   yAbove = ymax(r) - yres(r) * rowAbove - yres(r)/2
#   yBelow = yAbove - yres(r)
#   
#   m = matrix(ncol=2,byrow=T,data=c(xToLeft,yBelow,xToRight,yBelow,xToLeft,yAbove,xToRight,yAbove))
#   e = extract(r,vect(m),method="simple")[,2]
#   ll = e[1]
#   lr = e[2]
#   ul = e[3]
#   ur = e[4]
#   
#   linearinterp = function(lowcoord, highcoord, thiscoord, lowvalue, highvalue) {
#     interp = NA
#     if (!is.na(lowvalue) && !is.na(highvalue)) {
#       interp = (highcoord-thiscoord) / (highcoord-lowcoord) * lowvalue + (thiscoord-lowcoord) / (highcoord - lowcoord) * highvalue
#     } else if (!is.na(lowvalue) && is.na(highvalue)) {
#       interp = lowvalue
#     } else if (is.na(lowvalue) && !is.na(highvalue)) {
#       interp = highvalue
#     }
#     return(interp)
#   }
#   
#   directBelow = linearinterp(xToLeft,xToRight,x,ll,lr)
#   directAbove = linearinterp(xToLeft,xToRight,x,ul,ur)
#   return(linearinterp(yBelow,yAbove,y,directBelow,directAbove))
# }

writeRaster(dem,file.path(outputfolder,"dem.tif"),overwrite=T)

X = c()
Y = c()
Z = c()
ReturnNumber = c()
Classification = c()
Withheld_flag = c()
ScanAngle = c()


alignment = rast(xmin=0,xmax=100,ymin=0,ymax=100,res=10,crs="EPSG:32610+5703")

badclasses = c(7,9,18)
goodclasses = 0:20
goodclasses = goodclasses[!(goodclasses %in% badclasses)]

addPoints = function(n, x = runif(n,0,100), y = runif(n,0,100),
                     z = rep(10,n),returnnumber=rep(1,n),
                     classification = sample(goodclasses,n,T),
                     withheld_flag = rep(F,n),
                     scanangle = runif(n,-10,10)) {
  X <<- c(X,x)
  Y <<- c(Y,y)
  # zthis = c()
  # for (i in 1:length(z)) {
  #   zthis = c(zthis,z[i]+lapisBilinear(dem,x[i],y[i]))
  # }
  Z <<- c(Z,z+as.numeric(extract(dem,vect(cbind(x,y)),method="bilinear")[,2]))
  ReturnNumber <<- c(ReturnNumber,returnnumber)
  Classification <<- c(Classification,classification)
  Withheld_flag <<- c(Withheld_flag,withheld_flag)
  ScanAngle <<- c(ScanAngle,scanangle)
}

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

#putting it all into a las object
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
crs(l) = "EPSG:32610+5703"

#splitting the las into 10 so the testing can have multiple files to work with
start = 1
n = nrow(l@data)/10
for (i in 1:10) {
  part = l[start:(start+n-1),]
  writeLAS(part,file.path(outputfolder,paste0(i,".laz")))
  start = start + n
}

#writing the ini file that generates the run this is testing
ini = "#Data options
dem-units=unspecified
dem-algo=raster
las-units=unspecified
name=PointMetricsTest

#Processing options
xres=10
yres=10
xorigin=0
yorigin=0
csm-cellsize=1
footprint=0.4
smooth=3
minht=-8
maxht=100
class=~7,9,18
max-scan-angle=30
user-units=meters
canopy=1.5
strata=0.5,5.5,10.5,
min-tao-dist=0
id-algo=highpoint
seg-algo=watershed
topo-scale=500,1000,2000,
skip-csm=
skip-tao=
skip-topo=
adv-point=
"
ini = paste0(ini,"las=",outputfolder,"\n")
ini = paste0(ini,"dem=",outputfolder,"\n")
ini = paste0(ini,"output=",outputfolder,"Output")
sink(file.path(outputfolder,"params.ini"))
cat(ini)
sink(NULL)