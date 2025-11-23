cmake_minimum_required(VERSION 3.15)
project(MovieBookingService CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(GTest REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(include)

add_library(booking_lib
    src/BookingService.cpp
    src/DataStore.cpp
)
target_link_libraries(booking_lib nlohmann_json::nlohmann_json)

add_executable(movie_cli cli/main.cpp)
target_link_libraries(movie_cli booking_lib)

enable_testing()
add_executable(unit_tests tests/ServiceTests.cpp)
target_link_libraries(unit_tests booking_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(unit_tests)
