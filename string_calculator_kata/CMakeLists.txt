cmake_minimum_required (VERSION 3.0.0)
project (scalc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fprofile-arcs -ftest-coverage --coverage")

# Ensures the coverages files are named x.gcno as opposed to x.cpp.gcno
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

add_executable(scalc_tests src/string_calculator_kata.cc)

target_link_libraries (scalc_tests LINK_PUBLIC gmock pthread)

add_custom_command(TARGET scalc_tests 
	POST_BUILD
	COMMAND ./scalc_tests
	COMMAND lcov -c --no-external -b ../src -d . -o coverage.info --quiet
	COMMAND genhtml -o coverage -t "scalc Coverage Report" coverage.info	
	COMMAND gcovr -r ..
)

