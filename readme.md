# About Lapis

Lapis is an open-source program optimized for processing aerial lidar for forestry applications. By focusing on the most common processing jobs, instead of on flexibility, Lapis is able to run quickly and with a relatively low memory footprint. Lapis is designed to produce useful output in almost any situation, even for users who have little understanding of lidar and who are unclear on what the data they're processing contains, while still containing enough customization options to satisfy most common use cases.

# Current Status

Lapis is currently in alpha. It should compile (if cmake cooperates), and has been verified to run on a handful of lidar acquisitions, but I'm sure there are plenty of corner cases that will still cause it to crash. Furthermore, it's far from feature-complete: it still has only a command-line interface, produces no metadata output, and only produces a handful of gridded point metrics and a canopy surface model.

For a document listing planned features, see: https://docs.google.com/document/d/1cDbN9gfzf3-Cg4-rdte8ZD41vK1eSpStktrRzrUcJzo/edit?usp=sharing

# Building Lapis

Lapis is dependent on a number of packages: GDAL, PROJ, geotiff, xtl, laz-perf, and gtest (and possibly more to come as the project progresses). windowsbuildinstructions.txt has suggestions to help cmake find those on windows using conda.