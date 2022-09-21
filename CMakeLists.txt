cmake_minimum_required(VERSION 3.23)
project(Lapis VERSION 0.1)

if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.
")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(LAPIS_GIS_SOURCES
		src/gis/Alignment.cpp
		src/gis/Alignment.hpp
		src/gis/CoordRef.cpp
		src/gis/CoordRef.hpp
		src/gis/CoordTransform.cpp
		src/gis/CoordTransform.hpp
		src/gis/CoordVector.hpp
		src/gis/CropView.hpp
		src/gis/CurrentLasPoint.cpp
		src/gis/CurrentLasPoint.hpp
		src/gis/Extent.cpp
		src/gis/Extent.hpp
		src/gis/GDALWrappers.cpp
		src/gis/GDALWrappers.hpp
		src/gis/gis_pch.hpp
		src/gis/LasExtent.cpp
		src/gis/LasExtent.hpp
		src/gis/LasFilter.hpp
		src/gis/LasIO.cpp
		src/gis/LasIO.hpp
		src/gis/LasReader.cpp
		src/gis/LasReader.hpp
		src/gis/projwrappers.hpp
		src/gis/QuadExtent.cpp
		src/gis/QuadExtent.hpp
		src/gis/Raster.hpp
		src/gis/BaseDefs.hpp
		src/gis/Unit.hpp
		src/gis/RasterAlgos.hpp
		)

set(LAPIS_APP_SOURCES
		src/app_pch.hpp
		src/lapiscontroller.cpp
		src/lapiscontroller.hpp
		src/lapisutils.cpp
		src/lapisutils.hpp
		src/logger.cpp
		src/logger.hpp
		src/options.cpp
		src/options.hpp
		src/pointmetriccalculator.cpp
		src/pointmetriccalculator.hpp
		src/lapisobjects.hpp
		src/lapisobjects.cpp
		src/csmalgos.hpp
		src/csmalgos.cpp
		src/LapisPrivate.hpp
		src/LapisPrivate.cpp
		src/lapistypedefs.hpp
		src/lapisgui.hpp
		src/lapisgui.cpp
		)

set(LAPIS_TEST_SOURCES
		src/test/alignmenttest.cpp
		src/test/coordreftest.cpp
		src/test/coordtransformtest.cpp
		src/test/coordvectortest.cpp
		src/test/cropviewtest.cpp
		src/test/extenttest.cpp
		src/test/lasiotest.cpp
		src/test/lasreadertest.cpp
		src/test/quadextenttest.cpp
		src/test/rastertest.cpp
		src/test/lapisobjectstest.cpp
		src/test/pointmetriccalculatortest.cpp
		src/test/csmalgostest.cpp
		src/test/RasterAlgosTest.cpp
		)
		
set(LAPIS_IMGUI_SOURCES
		src/imgui/imgui.h
		src/imgui/imgui.cpp
		src/imgui/imgui_draw.cpp
		src/imgui/imgui_impl_glfw.h
		src/imgui/imgui_impl_glfw.cpp
		src/imgui/imgui_impl_opengl3.cpp
		src/imgui/imgui_impl_opengl3.h
		src/imgui/imgui_impl_opengl3_loader.h
		src/imgui/imgui_internal.h
		src/imgui/imgui_tables.cpp
		src/imgui/imgui_widgets.cpp
		src/imgui/imconfig.h
		src/imgui/imstb_rectpack.h
		src/imgui/imstb_textedit.h
		src/imgui/imstb_truetype.h
		)


add_library(Lapis_gis STATIC ${LAPIS_GIS_SOURCES})
add_library(Lapis_app STATIC ${LAPIS_APP_SOURCES})
add_library(Lapis_imgui STATIC ${LAPIS_IMGUI_SOURCES})

add_executable(Lapis src/lapis.cpp)
target_link_libraries(Lapis PUBLIC Lapis_gis)
target_link_libraries(Lapis PUBLIC Lapis_app)
target_link_libraries(Lapis PUBLIC Lapis_imgui)

add_executable(Lapis_test ${LAPIS_TEST_SOURCES})
target_link_libraries(Lapis_test PUBLIC Lapis_gis)
target_link_libraries(Lapis_test PUBLIC Lapis_app)

find_package(Boost 1.74 COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(Lapis PRIVATE ${Boost_LIBRARIES})
target_link_libraries(Lapis_test PRIVATE ${Boost_LIBRARIES})

find_package(GDAL 3.4 REQUIRED)
include_directories(${GDAL_INCLUDE_DIRS})
target_link_libraries(Lapis PRIVATE ${GDAL_LIBRARIES})
target_link_libraries(Lapis_test PRIVATE ${GDAL_LIBRARIES})

find_package(TIFF REQUIRED)

find_package(GeoTIFF 1.7 REQUIRED)
include_directories(${GEOTIFF_INCLUDE_DIRS})
target_link_libraries(Lapis PRIVATE ${GEOTIFF_LIBRARIES})
target_link_libraries(Lapis_test PRIVATE ${GEOTIFF_LIBRARIES})

find_package(PROJ 9.0 REQUIRED)
include_directories(${PROJ_INCLUDE_DIR})
target_link_libraries(Lapis PRIVATE ${PROJ_LIBRARIES})
target_link_libraries(Lapis_test PRIVATE ${PROJ_LIBRARIES})

find_package(lazperf 3.0 REQUIRED)
get_target_property(LAZPERF_INCLUDE_DIR LAZPERF::lazperf INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${LAZPERF_INCLUDE_DIR})
target_link_libraries(Lapis PRIVATE LAZPERF::lazperf)
target_link_libraries(Lapis_test PRIVATE LAZPERF::lazperf)

find_package(xtl 0.7 REQUIRED)
include_directories(${XTL_INCLUDE_DIR})

find_package(glfw3 3.3 REQUIRED)
target_link_libraries(Lapis PRIVATE glfw)

find_package(OpenGL REQUIRED)
target_link_libraries(Lapis PRIVATE OpenGL::GL)

add_subdirectory(src/nativefiledialog-extended)
include_directories(src/nativefiledialog-extended/src/include)
target_link_libraries(Lapis PRIVATE nfd)

find_package(gtest REQUIRED)

target_precompile_headers(Lapis_app PRIVATE src/app_pch.hpp)
target_precompile_headers(Lapis_gis PRIVATE src/gis/gis_pch.hpp)

if(MSVC)
  target_compile_options(Lapis PRIVATE /W3 /WX)
  target_compile_options(Lapis_app PRIVATE /W3 /WX)
  target_compile_options(Lapis_gis PRIVATE /W3 /WX)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
else()
  target_compile_options(Lapis PRIVATE -Wall -Wextra -Werror)
  target_compile_options(Lapis_app PRIVATE -Wall -Wextra -Werror)
  target_compile_options(Lapis_gis PRIVATE -Wall -Wextra -Werror)
endif()

add_compile_definitions(LAPISTESTFILES="${CMAKE_SOURCE_DIR}/src/test/testfiles/")

target_link_libraries(Lapis_test PRIVATE ${GTEST_BOTH_LIBRARIES})