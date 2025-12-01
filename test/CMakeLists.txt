cmake_minimum_required(VERSION 3.14)

project(luminaTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(lumina REQUIRED)
  enable_testing()
endif()

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(lumina_test source/lumina_test.cpp)
target_link_libraries(
    lumina_test PRIVATE
    lumina::lumina
    Catch2::Catch2WithMain
)
target_compile_features(lumina_test PRIVATE cxx_std_23)

catch_discover_tests(lumina_test)

# ---- End-of-file commands ----

add_folders(Test)
