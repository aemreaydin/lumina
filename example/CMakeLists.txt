cmake_minimum_required(VERSION 3.14)

project(luminaExamples CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(lumina REQUIRED)
endif()

add_custom_target(run-examples)

function(add_example NAME)
  add_executable("${NAME}" "${NAME}.cpp")
  target_link_libraries("${NAME}" PRIVATE lumina::lumina)
  target_compile_features("${NAME}" PRIVATE cxx_std_23)
  add_custom_target("run_${NAME}" COMMAND "${NAME}" VERBATIM)
  add_dependencies("run_${NAME}" "${NAME}")
  add_dependencies(run-examples "run_${NAME}")
endfunction()

add_example(sandbox)

# Copy config.toml to the example build directory
add_custom_command(
    TARGET sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.toml"
        "$<TARGET_FILE_DIR:sandbox>/config.toml"
    COMMENT "Copying config.toml to example directory"
)

add_folders(Example)
