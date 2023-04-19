# About Lapis

Lapis is an open-source program optimized for processing aerial lidar for forestry applications. By focusing on the most common processing jobs, instead of on flexibility, Lapis is able to run quickly, while still maintaining as small a memory footprint as reasonably possible. Lapis is designed to produce useful output in almost any situation, even for users who have little understanding of lidar and who are unclear on what the data they're processing contains, while still containing enough customization options to satisfy most common use cases.

# Current Status

Lapis is currently in alpha, version 0.7. It should compile (if cmake cooperates), and has been verified to run on a handful of lidar acquisitions, but I'm sure there are plenty of corner cases that will still cause it to crash. Furthermore, it's far from feature-complete. The following broad features are planned but not yet implemented:

- Increased customization for the canopy surface model, ground model, and tree segmentation algorithms.
- Improvements to the metadata file documenting each run.

# New in Version 0.7

- The average memory used should be lower now
- Calculating TPI was massively sped up. This should reduce the time when the program displays "Final processing and cleanup..." when topography is enabled
- Changed the default selection for DEM units from "same as horizontal units" to "same as LAS files". This should make it less likely for unit-related errors to exist
- Improved how the command line worked. It should now more closely resemble normal command line programs. In particular, the text output will not be captured if you run it via a program like Python or R.
- Fixed a bug where some layers wouldn't be calculated properly if you selected "first returns" in the point metrics tab
- Greatly sped up the calculation of some point metrics. You won't notice much difference if you select "common metrics" in the point metrics tab, but if you select "all metrics" it should be much much faster now
- Fixed a bug where sometimes, adjacent CSM tiles would have a one-pixel overlap
- Fixed a bug where canopy metrics near the edge of a tile would sometimes be calculated on a subset of the data they ought to be calculated on

# Getting Lapis

The latest release of Lapis is version 0.7. You can get the compiled binary here: 

Before running Lapis, you will need to install the Visual C++ Runtime Library, available here: https://aka.ms/vs/17/release/vc_redist.x64.exe

In future releases, that installation step will no longer be required.

# Known issues

- Lapis will crash suddenly if your machine runs out of memory. This is more likely to happen if you have less than 30 GB of memory, if your laz files are very large (>2 GB), or if your input data isn't tiled (such as if each file is a flightline).
- Certain projections, when specified in las versions 1.0 through 1.3, will cause oddities in the projection information in the output, and possibly interfere with Lapis' automatic unit detection. If you need to process a dataset with old laz files, consider specifying the projection and units of the data manually.

# Building Lapis

Lapis is developed on Windows, and is so far only tested on Windows using vcpkg as a dependency manager. Contributions to make it easier to build on other systems are welcome. Because I expect a number of users who aren't C++ developers to want to build Lapis from source, here are step by step instructions to compile Lapis on Windows. They assume familiarity with command line and with git, but no experience with C++. More compact instructions designed for people familiar with C++ are further down.

 - First, install Visual Studio, available at https://visualstudio.microsoft.com/
 - When the installer asks what you intend to use Visual Studio for, select the box saying C/C++ development
 - Install git, and add it to the system path if desired
 - Install vcpkg, available at https://vcpkg.io/en/getting-started.html, and follow the instructions
 - Run the following command to install Lapis' dependencies. This may take several hours. vcpkg install --clean-after-build --triplet=x64-windows boost gdal proj libgeotiff xtl gtest glfw3 libharu
 - Clone the Lapis repository with the --recurse-submodules flag. The command should be something like: git clone https://github.com/jontkane/Lapis.git --recurse-submodules
 - If you didn't already have cmake installed, inside the place where you cloned vcpkg, there should be a file like vcpkg\downloads\tools\cmake-3.25.0-windows\cmake-3.25.0-windows-i386\bin\cmake.exe. This may be slightly different if a new version of cmake releases after these instructions are written. Add the folder containing cmake.exe to the system path.
 - Lapis requires the files proj.ini and proj.db to be in the same directory as the executable. The file copyprojandcmake.bat will copy those files to where they need to be, and then run cmake with the correct parameters. Right click->edit that bat file, and edit the first two lines to indicate the folders you cloned Lapis and vcpkg into. Then save it and run the batch file.
 - If there were no cmake errors, then there should now be a file inside the build folder in the Lapis directory called "Lapis.sln". Open it with visual studio. In the top-center of the screen, there should be a drop-down menu which currently says "debug". Change it to "RelWithDebInfo"
 - Press F5 to build Lapis and then start it. Enjoy!
 
 When you want to update Lapis, do a git pull, and then rerun the bat file from above.
 
 The short version of the instructions, for those familiar with C++, are:
 
 - Clone Lapis using --recurse-submodules
 - Install Lapis' dependencies using vcpkg. The following command should work: vcpkg install --clean-after-build --triplet=x64-windows boost gdal proj libgeotiff xtl gtest glfw3 libharu
 - Do a cmake call something like: cmake .. -DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
 - Make sure proj.ini and proj.db (both will be in your vcpkg folder) end up in the same directory as the Lapis executable