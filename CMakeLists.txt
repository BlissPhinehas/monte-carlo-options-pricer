cmake_minimum_required(VERSION 3.14)
project(MonteCarloPricer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(pricer_lib
    src/BlackScholes.cpp
    src/PathSimulator.cpp
    src/EuropeanPricer.cpp
    src/AsianPricer.cpp
)
target_include_directories(pricer_lib PUBLIC include)

add_executable(pricer src/main.cpp)
target_link_libraries(pricer PRIVATE pricer_lib)

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(run_tests tests/test_pricer.cpp)
target_link_libraries(run_tests PRIVATE pricer_lib GTest::GTest GTest::Main)

gtest_discover_tests(run_tests)
