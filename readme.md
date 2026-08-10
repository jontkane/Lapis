# About Lapis

Lapis is an open-source program optimized for processing aerial lidar for forestry applications. By focusing on the most common processing jobs, instead of on flexibility, Lapis is able to run quickly, while still maintaining as small a memory footprint as reasonably possible. Lapis is designed to produce useful output in almost any situation, even for users who have little understanding of lidar and who are unclear on what the data they're processing contains, while still containing enough customization options to satisfy most common use cases.

View a video presentation on Lapis at:

https://www.youtube.com/watch?v=MJ2JBvHFBek&ab_channel=LapisLidar

# Current Status

Lapis is currently in alpha, version 0.9. It should compile (if cmake cooperates), and has been verified to run on many lidar acquisitions, but there may be bugs remaining.

# New in Version 0.9

- Many, many bug fixes
- Many speedups
- Better utilization of memory
- Subsetting a run to a rectangular extent
- Multiple enhancements to the tree segmentation output

# Getting Lapis

The latest release of Lapis is version 0.9. See releases for a binary.

Before running Lapis, you will need to install the Visual C++ Runtime Library, available here: https://aka.ms/vs/17/release/vc_redist.x64.exe

In future releases, that installation step will no longer be required.

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
 - The file examplecmake.bat has an example of the correct syntax to run cmake with to build Lapis
 - If there were no cmake errors, then there should now be a file inside the build folder in the Lapis directory called "Lapis.sln". Open it with visual studio. In the top-center of the screen, there should be a drop-down menu which currently says "debug". Change it to "RelWithDebInfo"
 - Press F5 to build Lapis and then start it. Enjoy!
 
 When you want to update Lapis, do a git pull, and then rerun the bat file from above.
 
 The short version of the instructions, for those familiar with C++, are:
 
 - Clone Lapis using --recurse-submodules
 - Install Lapis' dependencies using vcpkg. The following command should work: vcpkg install --clean-after-build --triplet=x64-windows boost gdal proj libgeotiff xtl gtest glfw3 libharu
 - Do a cmake call something like: cmake .. -DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DPROJ_DB_PATH=vcpkg\installed\x64-windows\share\proj\proj.db