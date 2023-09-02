# About Lapis

Lapis is an open-source program optimized for processing aerial lidar for forestry applications. By focusing on the most common processing jobs, instead of on flexibility, Lapis is able to run quickly, while still maintaining as small a memory footprint as reasonably possible. Lapis is designed to produce useful output in almost any situation, even for users who have little understanding of lidar and who are unclear on what the data they're processing contains, while still containing enough customization options to satisfy most common use cases.

View a video presentation on Lapis at:

https://www.youtube.com/watch?v=MJ2JBvHFBek&ab_channel=LapisLidar

# Current Status

Lapis is currently in alpha, version 0.8. It should compile (if cmake cooperates), and has been verified to run on a handful of lidar acquisitions, but I'm sure there are plenty of corner cases that will still cause it to crash. Furthermore, it's far from feature-complete. The following broad features are planned but not yet implemented:

- Increased customization for the canopy surface model, ground model, and tree segmentation algorithms.
- Improvements to the metadata file documenting each run.

# New in Version 0.8

- A much larger suite of topographic metrics
- A number of bug fixes
- Improvements to the GUI
- Improvements to automatic unit detection, and better communication on what units Lapis thinks the data is in.

# Getting Lapis

The latest release of Lapis is version 0.8. You can get the compiled binary here: https://drive.google.com/file/d/1DQzt9T-Wjj_wjJUvUSA8pg-Lqn3l6RzW/view?usp=drive_link

Before running Lapis, you will need to install the Visual C++ Runtime Library, available here: https://aka.ms/vs/17/release/vc_redist.x64.exe

In future releases, that installation step will no longer be required.

# Known issues

- Lapis will crash suddenly if your machine runs out of memory. This is more likely to happen if you have less than 30 GB of memory, if your laz files are very large (>2 GB), or if your input data isn't tiled (such as if each file is a flightline).
- 

# Updates and Bug Reports

If you want to be notified when a new version of Lapis is released, please contact the developer at jontkane@uw.edu. In future versions, there may be automatic updates.

If you encounter a bug, please report it either on github or at that email address.

# Building Lapis

Lapis is developed on Windows, and is so far only tested on Windows using vcpkg as a dependency manager. Contributions to make it easier to build on other systems are welcome. Because I expect some users who aren't C++ developers to want to build Lapis from source, here are step by step instructions to compile Lapis on Windows. They assume familiarity with command line and with git, but no experience with C++. More compact instructions designed for people familiar with C++ are further down.

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