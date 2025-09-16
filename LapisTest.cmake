include_guard(GLOBAL)

if (NOT (DEFINED LAPIS_DIR AND LAPIS_DIR))
	message(FATAL_ERROR
		"LapisTests.cmake should not be run before Lapis.cmake"
		)
endif()

# Option to allow skipping R-based tests if R is not available
option(LAPIS_SKIP_TESTS "Build without tests (R is no longer required)" OFF)

if (LAPIS_SKIP_TESTS)
	message("Skipping Lapis tests")
	return()
endif()

# Try to find Rscript in the PATH
find_program(RSCRIPT_EXECUTABLE NAMES Rscript)

if(NOT RSCRIPT_EXECUTABLE)
	message(FATAL_ERROR
		"Rscript not found! R is required for tests.\n"
		"If you want to build without tests, re-run cmake with:\n"
		"    -DLAPIS_SKIP_TESTS=ON"
	)
endif()

file(GLOB LAPIS_TEST_SOURCES
	${LAPIS_DIR}/src/test/*.cpp
	${LAPIS_DIR}/src/test/*.hpp)
add_executable(Lapis_test ${LAPIS_TEST_SOURCES})
copy_proj_db_after_build(Lapis_test)

target_include_directories(Lapis_test PRIVATE ${LAPIS_EXTERNAL_INCLUDES})
target_link_libraries(Lapis_test PRIVATE ${LAPIS_EXTERNAL_LINKS})
target_link_libraries(Lapis_test PRIVATE ${LAPIS_INTERNAL_LINKS})

find_package(GTest REQUIRED)
target_include_directories(Lapis_test PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(Lapis_test PRIVATE ${GTEST_LIBRARIES})

find_package(yaml-cpp REQUIRED)
target_link_libraries(Lapis_test PRIVATE yaml-cpp)

target_precompile_headers(Lapis_test PRIVATE ${LAPIS_DIR}/src/test/test_pch.hpp)

if (MSVC)
	target_compile_options(Lapis_test PRIVATE /W3 /WX)
else()
	target_compile_options(Lapis_test PRIVATE -Wall -WExtra -Werror)
endif()	

set(R_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/src/test/testdata/RunAllTestScripts.R")
set(TESTDATA_INPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/test/testdata")
set(TESTDATA_OUTPUT_DIR "${CMAKE_BINARY_DIR}/testdata_output")

file(GLOB TEST_INPUTS
	"${TESTDATA_INPUT_DIR}/*.tif"
	"${TESTDATA_INPUT_DIR}/*.laz")
add_custom_command(
    OUTPUT ${TESTDATA_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TESTDATA_OUTPUT_DIR}
    COMMAND ${RSCRIPT_EXECUTABLE} ${R_SCRIPT} ${TESTDATA_INPUT_DIR} ${TESTDATA_OUTPUT_DIR}
    DEPENDS ${R_SCRIPT} ${TEST_INPUTS}
    COMMENT "Generating expected outputs with LidR"
)
add_custom_target(run_r ALL DEPENDS ${TESTDATA_OUTPUT_DIR})

add_dependencies(Lapis_test run_r)

target_compile_definitions(Lapis_test PRIVATE
    TESTDATA_INPUT_DIR="${TESTDATA_INPUT_DIR}"
    TESTDATA_OUTPUT_DIR="${TESTDATA_OUTPUT_DIR}"
)

file(GLOB ALL_R_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/src/test/testdata/*.R")
file(GLOB ALL_YAML "${CMAKE_CURRENT_SOURCE_DIR}/src/test/testdata/*.yaml")
list(APPEND LAPIS_SCRIPTS
    ${CMAKE_CURRENT_LIST_FILE}
	${ALL_R_SCRIPTS}
	${ALL_YAML}
)






