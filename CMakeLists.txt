cmake_minimum_required(VERSION 3.28.0)
PROJECT(2026_C_AV_PRC)
SET(CMAKE_C_FLAGS "-g -Wall -Werror")
SET(CMAKE_CXX_FLAGS "-g -Wall -Werror -std=c++20")

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
        COMMAND lcov --remove coverage.info '/usr/*' '*test*' 'SignalUtils*' --output-file coverage_filtered.info --rc branch_coverage=1

        COMMAND genhtml coverage_filtered.info --output-directory html_report --rc branch_coverage=1
    )
endif()

#############################################################
######################## GOOGLE TEST ########################
#############################################################

include(FetchContent)
find_package(OpenSSL REQUIRED)

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
    target_link_libraries(${test_name} PRIVATE GTest::gtest_main GTest::gmock OpenSSL::Crypto)
    gtest_discover_tests(${test_name})

    target_include_directories(${test_name} PUBLIC . ThirdParty/SignalUtils/include)
endfunction()

create_test(test_average_module Modules/Sensors/tests/test_average_policy.cpp)
create_test(test_outlier_module Modules/Sensors/tests/test_outlier_policy.cpp)

create_test(test_base_logger ThirdParty/DataLogger/Tests/test_base_logger.cpp)

create_test(test_engine_state_igniter Application/FlightControl/tests/test_engine_state_igniter.cpp)
target_sources(test_engine_state_igniter PRIVATE
    Application/FlightControl/engine_state.cpp
    Application/Config/config.cpp
    Application/Config/2026_C_AV_FLIGHT_PARAMS/FlightParams.cpp
    Application/Data/data.cpp
    Application/Data/Stores/BoardIdentityStore.cpp
    Application/Data/Stores/EventStore.cpp
    Application/Data/Stores/IntranetCmdStore.cpp
    Application/Data/Stores/ValvesStore.cpp
    Application/Data/propulsion/store.cpp
    Drivers/Plume/2026_C_AV_PLUME/src/plume/status.c)
target_include_directories(test_engine_state_igniter PRIVATE
    Application/FlightControl/tests/shim
    Application/Config/2026_C_AV_FLIGHT_PARAMS
    Application/Config/2026_C_AV_FLIGHT_PARAMS/2026_C_AV_CONFMAN
    Drivers/Plume/2026_C_AV_PLUME/include
    Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/include)
target_compile_options(test_engine_state_igniter PRIVATE -Wno-unused-function -include stddef.h)

# Manual MO, ME and igniter gate and the FSM taking the outputs back, same shim.
create_test(test_engine_state_manual Application/FlightControl/tests/test_engine_state_manual.cpp)
get_target_property(ENGINE_STATE_TEST_SOURCES test_engine_state_igniter SOURCES)
list(REMOVE_ITEM ENGINE_STATE_TEST_SOURCES Application/FlightControl/tests/test_engine_state_igniter.cpp)
target_sources(test_engine_state_manual PRIVATE ${ENGINE_STATE_TEST_SOURCES})
target_include_directories(test_engine_state_manual PRIVATE
    Application/FlightControl/tests/shim
    Application/Config/2026_C_AV_FLIGHT_PARAMS
    Application/Config/2026_C_AV_FLIGHT_PARAMS/2026_C_AV_CONFMAN
    Drivers/Plume/2026_C_AV_PLUME/include
    Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/include)
target_compile_options(test_engine_state_manual PRIVATE -Wno-unused-function -include stddef.h)

# USB shell line assembler and dispatcher, against a fake prc_cli driver.
# prc_commands.cpp (the HAL bound handlers) is deliberately not linked.
create_test(test_prc_shell Application/Tests/test_prc_shell.cpp)
target_sources(test_prc_shell PRIVATE
    Application/FlightControl/prc_shell.cpp
    Application/app_printf.cpp
    Drivers/CLI/2026_C_AV_CLI/prc_commands_generated.cpp)
target_include_directories(test_prc_shell PUBLIC Drivers/CLI/2026_C_AV_CLI)

# Pure command policies from prc_commands.hpp (no HAL).
create_test(test_prc_commands Application/Tests/test_prc_commands.cpp)
target_include_directories(test_prc_commands PUBLIC Drivers/CLI/2026_C_AV_CLI)
