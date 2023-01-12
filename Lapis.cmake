file(GLOB LAPIS_GIS_SOURCES
	${LAPIS_DIR}/src/gis/*.hpp
	${LAPIS_DIR}/src/gis/*.cpp)

file(GLOB LAPIS_PARAMETERS_SOURCES
	${LAPIS_DIR}/src/parameters/*.hpp
	${LAPIS_DIR}/src/parameters/*.cpp)

file(GLOB LAPIS_RUN_SOURCES
	${LAPIS_DIR}/src/run/*.hpp
	${LAPIS_DIR}/src/run/*.cpp)

file(GLOB LAPIS_LOGGER_SOURCES
	${LAPIS_DIR}/src/logger/*.cpp
	${LAPIS_DIR}/src/logger/*.hpp)

file(GLOB LAPIS_ALGO_SOURCES
	${LAPIS_DIR}/src/algorithms/*.cpp
	${LAPIS_DIR}/src/algorithms/*.hpp)

file(GLOB LAPIS_TEST_SOURCES
	${LAPIS_DIR}/src/test/*.cpp
	${LAPIS_DIR}/src/test/*.hpp)

file(GLOB LAPIS_IMGUI_SOURCES
	${LAPIS_DIR}/src/imgui/*.cpp
	${LAPIS_DIR}/src/imgui/*.h)

set(LAPIS_EXE_SOURCES
	${LAPIS_DIR}/src/Lapis.cpp
	${LAPIS_DIR}/src/Lapis.hpp
	${LAPIS_DIR}/src/LapisTypeDefs.hpp
	)

add_executable(Lapis WIN32 ${LAPIS_EXE_SOURCES})
add_executable(Lapis_test ${LAPIS_TEST_SOURCES})

add_library(Lapis_gis STATIC ${LAPIS_GIS_SOURCES})
add_library(Lapis_algorithms STATIC ${LAPIS_ALGO_SOURCES})
add_library(Lapis_params OBJECT ${LAPIS_PARAMETERS_SOURCES})
add_library(Lapis_run OBJECT ${LAPIS_RUN_SOURCES})
add_library(Lapis_logger STATIC ${LAPIS_LOGGER_SOURCES})
add_library(Lapis_imgui STATIC ${LAPIS_IMGUI_SOURCES})

find_package(GDAL REQUIRED)
find_package(Boost COMPONENTS program_options REQUIRED)
find_package(GeoTIFF REQUIRED)
find_package(PROJ REQUIRED)
find_package(xtl REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PoDoFo REQUIRED)

add_subdirectory(${LAPIS_DIR}/src/nativefiledialog-extended nfd)
#not using add_subdirectory here because lazperf generates a very annoying number of targets
file(GLOB_RECURSE LAZPERF_FILES 
	${LAPIS_DIR}/src/lazperf/cpp/lazperf/*.cpp
	${LAPIS_DIR}/src/lazperf/cpp/lazperf/*.hpp
)
add_library(lazperf STATIC ${LAZPERF_FILES})

set(LAPIS_EXTERNAL_INCLUDES
	${GDAL_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS}
	${GeoTIFF_INCLUDE_DIRS}
	${PROJ_INCLUDE_DIR}
	${xtl_INCLUDE_DIR}
	${glfw_INCLUDE_DIRS}
	${OpenGL_INCLUDE_DIRS}
	${LAPIS_DIR}/src/nativefiledialog-extended/src/include
	${LAPIS_DIR}/src/lazperf/cpp
	)

set(LAPIS_EXTERNAL_LINKS
	${GDAL_LIBRARIES}
	${Boost_LIBRARIES}
	${GeoTIFF_LIBRARIES}
	${PROJ_LIBRARIES}
	glfw
	OpenGL::GL
	nfd
	lazperf
	)

set(LAPIS_INTERNAL_LINKS
	Lapis_gis
	Lapis_algorithms
	Lapis_params
	Lapis_run
	Lapis_logger
	Lapis_imgui
	)
	
target_include_directories(Lapis_gis PRIVATE ${LAPIS_EXTERNAL_INCLUDES})
target_include_directories(Lapis_algorithms PRIVATE ${LAPIS_EXTERNAL_INCLUDES})
target_include_directories(Lapis_params PRIVATE ${LAPIS_EXTERNAL_INCLUDES})
target_include_directories(Lapis_run PRIVATE ${LAPIS_EXTERNAL_INCLUDES})
target_include_directories(Lapis_logger PRIVATE ${LAPIS_EXTERNAL_INCLUDES})
target_include_directories(Lapis_imgui PRIVATE ${LAPIS_EXTERNAL_INCLUDES})

target_include_directories(Lapis PRIVATE ${LAPIS_EXTERNAL_INCLUDES})
target_link_libraries(Lapis PRIVATE ${LAPIS_EXTERNAL_LINKS})
target_link_libraries(Lapis PRIVATE ${LAPIS_INTERNAL_LINKS})

target_include_directories(Lapis_test PRIVATE ${LAPIS_EXTERNAL_INCLUDES})
target_link_libraries(Lapis_test PRIVATE ${LAPIS_EXTERNAL_LINKS})
target_link_libraries(Lapis_test PRIVATE ${LAPIS_INTERNAL_LINKS})


find_package(GTest REQUIRED)
target_include_directories(Lapis_test PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(Lapis_test PRIVATE ${GTEST_BOTH_LIBRARIES})

target_precompile_headers(Lapis_params PRIVATE ${LAPIS_DIR}/src/parameters/param_pch.hpp)
target_precompile_headers(Lapis_run PRIVATE ${LAPIS_DIR}/src/run/run_pch.hpp)
target_precompile_headers(Lapis_gis PRIVATE ${LAPIS_DIR}/src/gis/gis_pch.hpp)
target_precompile_headers(Lapis_algorithms PRIVATE ${LAPIS_DIR}/src/algorithms/algo_pch.hpp)
target_precompile_headers(Lapis_test PRIVATE ${LAPIS_DIR}/src/test/test_pch.hpp)

if (MSVC)
	target_compile_options(Lapis PRIVATE /W3 /WX)
	target_compile_options(Lapis_logger PRIVATE /W3 /WX)
	target_compile_options(Lapis_params PRIVATE /W3 /WX)
	target_compile_options(Lapis_algorithms PRIVATE /W3 /WX)
	target_compile_options(Lapis_run PRIVATE /W3 /WX)
	target_compile_options(Lapis_gis PRIVATE /W3 /WX)
	target_compile_options(Lapis_test PRIVATE /W3 /WX)
	target_compile_options(lazperf PRIVATE /W0)
	target_compile_options(nfd PRIVATE /W0)
	target_compile_options(Lapis_imgui PRIVATE /W0)
else()
	target_compile_options(Lapis PRIVATE -Wall -Wextra -Werror)
	target_compile_options(Lapis_logger PRIVATE -Wall -Wextra -Werror)
	target_compile_options(Lapis_params PRIVATE -Wall -Wextra -Werror)
	target_compile_options(Lapis_algorithms PRIVATE -Wall -Wextra -Werror)
	target_compile_options(Lapis_run PRIVATE -Wall -Wextra -Werror)
	target_compile_options(Lapis_gis PRIVATE -Wall -Wextra -Werror)
	target_compile_options(Lapis_test PRIVATE -Wall -WExtra -Werror)
endif()

add_compile_definitions(LAPISTESTFILES="${LAPIS_DIR}/src/test/testfiles/")