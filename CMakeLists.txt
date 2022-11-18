cmake_minimum_required(VERSION 3.24)
project(Lapis VERSION 0.1)

if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.
")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(VCPKG_TARGET_TRIPLET x64-windows)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

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
		src/LapisLogger.cpp
		src/LapisLogger.hpp
		src/pointmetriccalculator.cpp
		src/pointmetriccalculator.hpp
		src/csmalgos.hpp
		src/csmalgos.cpp
		src/LapisPrivate.hpp
		src/LapisPrivate.cpp
		src/lapistypedefs.hpp
		src/lapisgui.hpp
		src/lapisgui.cpp
		src/LapisParameters.hpp
		src/LapisParameters.cpp
		src/LapisData.hpp
		src/LapisData.cpp
		src/MetricTypeDefs.hpp
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
		src/test/pointmetriccalculatortest.cpp
		src/test/csmalgostest.cpp
		src/test/RasterAlgosTest.cpp
		src/test/lapisdatatest.cpp
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

add_executable(Lapis WIN32 src/lapis.cpp)
target_link_libraries(Lapis PUBLIC Lapis_gis)
target_link_libraries(Lapis PUBLIC Lapis_app)
target_link_libraries(Lapis PUBLIC Lapis_imgui)

add_executable(Lapis_test ${LAPIS_TEST_SOURCES})
target_link_libraries(Lapis_test PUBLIC Lapis_gis)
target_link_libraries(Lapis_test PUBLIC Lapis_app)
target_link_libraries(Lapis_test PUBLIC Lapis_imgui)

find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIRS})
target_link_libraries(Lapis PRIVATE ${GDAL_LIBRARIES})
target_link_libraries(Lapis_test PRIVATE ${GDAL_LIBRARIES})

find_package(Boost COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(Lapis PRIVATE ${Boost_LIBRARIES})
target_link_libraries(Lapis_test PRIVATE ${Boost_LIBRARIES})

find_package(GeoTIFF REQUIRED)
include_directories(${GeoTIFF_INCLUDE_DIRS})
target_link_libraries(Lapis PRIVATE ${GeoTIFF_LIBRARIES})
target_link_libraries(Lapis_test PRIVATE ${GeoTIFF_LIBRARIES})

find_package(PROJ REQUIRED)
include_directories(${PROJ_INCLUDE_DIR})
target_link_libraries(Lapis PRIVATE ${PROJ_LIBRARIES})
target_link_libraries(Lapis_test PRIVATE ${PROJ_LIBRARIES})

find_package(xtl REQUIRED)
include_directories(${xtl_INCLUDE_DIR})

find_package(glfw3 REQUIRED)
include_directories(${glfw_INCLUDE_DIRS})
target_link_libraries(Lapis PRIVATE glfw)
target_link_libraries(Lapis_test PRIVATE glfw)

find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
target_link_libraries(Lapis PRIVATE OpenGL::GL)
target_link_libraries(Lapis_test PRIVATE OpenGL::GL)

add_subdirectory(src/nativefiledialog-extended)
include_directories(src/nativefiledialog-extended/src/include)
target_link_libraries(Lapis PRIVATE nfd)
target_link_libraries(Lapis_test PRIVATE nfd)

find_package(GTest REQUIRED)
target_include_directories(Lapis_test PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(Lapis_test PRIVATE ${GTEST_BOTH_LIBRARIES})

#not using add_subdirectory here because lazperf generates a very annoying number of targets
include_directories(src/lazperf/cpp)
file(GLOB_RECURSE LAZPERF_FILES 
	src/lazperf/cpp/lazperf/*.cpp
	src/lazperf/cpp/lazperf/*.hpp
)
add_library(lazperf STATIC ${LAZPERF_FILES})
target_link_libraries(Lapis PRIVATE lazperf)
target_link_libraries(Lapis_test PRIVATE lazperf)

target_precompile_headers(Lapis_app PRIVATE src/app_pch.hpp)
target_precompile_headers(Lapis_gis PRIVATE src/gis/gis_pch.hpp)

if(MSVC)
  target_compile_options(Lapis PRIVATE /W3 /WX)
  target_compile_options(Lapis_app PRIVATE /W3 /WX)
  target_compile_options(Lapis_gis PRIVATE /W3 /WX)
  target_compile_options(Lapis_test PRIVATE /W3 /WX)
  target_compile_options(lazperf PRIVATE /W0)
  target_compile_options(nfd PRIVATE /W0)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Lapis)
else()
  target_compile_options(Lapis PRIVATE -Wall -Wextra -Werror)
  target_compile_options(Lapis_app PRIVATE -Wall -Wextra -Werror)
  target_compile_options(Lapis_gis PRIVATE -Wall -Wextra -Werror)
  target_compile_options(Lapis_test PRIVATE -Wall -WExtra -Werror)
endif()

add_compile_definitions(LAPISTESTFILES="${CMAKE_SOURCE_DIR}/src/test/testfiles/")