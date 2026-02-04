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

add_example(triangle)
add_example(texture)
add_example(depth)
add_example(scene_demo)
add_example(rendergraph_demo)

foreach(EXAMPLE_TARGET triangle texture depth scene_demo rendergraph_demo)
    add_custom_command(
        TARGET ${EXAMPLE_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/config.toml"
            "$<TARGET_FILE_DIR:${EXAMPLE_TARGET}>/config.toml"
        COMMENT "Copying config.toml to ${EXAMPLE_TARGET} directory"
    )

    add_custom_command(
        TARGET ${EXAMPLE_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "$<TARGET_FILE_DIR:${EXAMPLE_TARGET}>/shaders"
        COMMENT "Copying shaders to ${EXAMPLE_TARGET} directory"
    )

    add_custom_command(
        TARGET ${EXAMPLE_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:${EXAMPLE_TARGET}>/assets"
        COMMENT "Copying assets to ${EXAMPLE_TARGET} directory"
    )

    add_custom_command(
        TARGET ${EXAMPLE_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/third_party/fonts"
            "$<TARGET_FILE_DIR:${EXAMPLE_TARGET}>/fonts"
        COMMENT "Copying fonts to ${EXAMPLE_TARGET} directory"
    )
endforeach()

add_folders(Example)
