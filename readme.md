# About Lapis

Lapis is an open-source program optimized for processing aerial lidar for forestry applications. By focusing on the most common processing jobs, instead of on flexibility, Lapis is able to run quickly, while still maintaining as small a memory footprint as reasonably possible. Lapis is designed to produce useful output in almost any situation, even for users who have little understanding of lidar and who are unclear on what the data they're processing contains, while still containing enough customization options to satisfy most common use cases.

# Current Status

Lapis is currently in alpha. It should compile (if cmake cooperates), and has been verified to run on a handful of lidar acquisitions, but I'm sure there are plenty of corner cases that will still cause it to crash. Furthermore, it's far from feature-complete. The following broad features are planned but not yet implemented:

Increased customization for the canopy surface model, ground model, and tree segmentation algorithms.
Metadata output documenting the output produced by a run.
Features to check for common problems in input data before the user wastes time on a doomed run.


# Building Lapis

Lapis is dependent on a number of packages. The ones that are more difficult to find automatically with cmake are embedded into this repository; the others will have to be acquired separately. windowsbuildinstructions.txt has suggestions to help cmake find those on windows using conda.

No compiled binaries are provided at this point.