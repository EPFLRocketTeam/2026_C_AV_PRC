cmake_minimum_required(VERSION 3.28.0)
PROJECT(2026_C_AV_PRC__SIL)
SET(CMAKE_C_FLAGS "-g -Wall -Werror")
SET(CMAKE_CXX_FLAGS "-g -Wall -Werror -std=c++17")

option(ENABLE_COVERAGE "Enable code coverage instrumentation (requires gcov/lcov)" OFF)
find_program(VALGRIND_EXECUTABLE NAMES valgrind)
add_compile_definitions(UNIT_TEST_ENV)

if (ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -O0")

    set(MEMORYCHECK_COMMAND "${VALGRIND_EXECUTABLE}")
    set(MEMORYCHECK_COMMAND_OPTIONS "--tool=memcheck --leak-check=full --track-fds=yes --error-exitcode=1")

    add_custom_target(
        memcheck

        COMMAND ctest -T memcheck
    )
    add_custom_target(
        coverage

        COMMAND lcov --directory . --zerocounters --rc branch_coverage=1
        COMMAND ctest

        COMMAND lcov --capture --directory . --output-file coverage.info --ignore-errors mismatch --rc branch_coverage=1
        COMMAND lcov --remove coverage.info '/usr/*' '*test*' --output-file coverage_filtered.info --rc branch_coverage=1

        COMMAND genhtml coverage_filtered.info --output-directory html_report --rc branch_coverage=1
    )
endif()


#############################################################
#################### PROPULSION COMPUTER ####################
#############################################################

# Add your source files here
# e.g. Drivers/X/Impl/X_mock.cpp
SET(PRC_SOURCE_FILES
    Application/main.cpp
    
    Drivers/STM32HAL/stm32sim_spi.cpp
    Drivers/STM32HAL/stm32sim_gpio.cpp
    Drivers/KULITE_CTL190/Impl/KULITE_CTL190.cpp)

add_library(propulsion_computer STATIC ${PRC_SOURCE_FILES})
target_link_libraries(propulsion_computer PRIVATE gcov)
target_include_directories(propulsion_computer PUBLIC .)
target_include_directories(propulsion_computer PUBLIC Drivers/KULITE_CTL190)

# Add includes here
# For example target_include_directories(propulsion_computer PUBLIC Drivers/X)

#############################################################
######################## GOOGLE TEST ########################
#############################################################

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

include(CTest)
set(CTEST_TIMEOUT 30)

include(GoogleTest)

function(create_test test_name test_file)
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} PRIVATE propulsion_computer GTest::gtest_main GTest::gmock)
    gtest_discover_tests(${test_name})
endfunction()

add_subdirectory(Drivers/STM32HAL/Tests)

