library(terra)
library(lidR)
library(yaml)

#Synthetic raster and point cloud that don't do anything fancy
r = rast(nrows=100, ncols=100, xmin=0, xmax=100, ymin=0, ymax=100)
values(r) = 100 + 10 * sin(xFromCell(r, 1:ncell(r)) / 10) + 5 * cos(yFromCell(r, 1:ncell(r)) / 10)
writeRaster(r, file.path(outputfolder,"synthetic_groundmodel_simple.tif"), overwrite=T)

set.seed(0)
npoints = 10000
x = runif(npoints, 0, 100)
y = runif(npoints, 0, 100)
groundz = extract(r, cbind(x, y))[,1]
dz = runif(npoints, -0.5, 60)  # Height above ground
z = groundz + dz

las = LAS(data.frame(X = x, Y = y, Z = z, Intensity = sample(1:255, npoints, replace=T)))
writeLAS(las, file.path(outputfolder,"synthetic_pointcloud_simple.laz"))


#synthetic raster and point cloud with holes
set.seed(1)
r = rast(nrows=100, ncols=100, xmin=0, xmax=100, ymin=0, ymax=100)
values(r) = 120 + 8 * sin(xFromCell(r, 1:ncell(r)) / 7) * cos(yFromCell(r, 1:ncell(r)) / 8) +
                  0.2 * xFromCell(r, 1:ncell(r)) - 0.1 * yFromCell(r, 1:ncell(r))


nholes = 10
holeradius = 2 #not really a radius, because the holes are squares. A half-width
holex = runif(nholes, xmin(r), xmax(r))
holey = runif(nholes, ymin(r), ymax(r))
npoints = 10000
x = runif(npoints, 0, 100)
y = runif(npoints, 0, 100)
excludex = apply(data.frame(lapply(holex,function(a){abs(x-a)})),1,min) < holeradius
excludey = apply(data.frame(lapply(holey,function(a){abs(y-a)})),1,min) < holeradius
exclude = excludex & excludey
x = x[!exclude]
y = y[!exclude]

groundz = extract(r, cbind(x, y))[,1]
dz = runif(length(x), -0.5, 60)
z = groundz + dz

las = LAS(data.frame(X = x, Y = y, Z = z, Intensity = sample(1:255, length(x), replace=T)))
writeLAS(las, file.path(outputfolder,"synthetic_pointcloud_with_holes.laz"))

#putting the holes in the raster last so the normalization step works
holecells = sample(1:ncell(r), 100)
r[holecells] = NA
writeRaster(r, file.path(outputfolder,"synthetic_groundmodel_with_holes.tif"), overwrite=T)

file_associations = list(
  list(
    shortname = "simple",
    tif = list(file.path(outputfolder, "synthetic_groundmodel_simple.tif")),
    laz = file.path(outputfolder, "synthetic_pointcloud_simple.laz")
  ),
  list(
    shortname = "holes",
    tif = list(file.path(outputfolder, "synthetic_groundmodel_with_holes.tif")),
    laz = file.path(outputfolder, "synthetic_pointcloud_with_holes.laz")
  ),
  list(
    shortname = "real",
    tif = list(file.path(inputfolder, "real_data_left.tif"),file.path(inputfolder,"real_data_right_projected.tif")),
    laz = file.path(inputfolder, "real_data.laz")
  )
)

write_yaml(file_associations, file.path(outputfolder, "tif_laz_associations.yaml"))