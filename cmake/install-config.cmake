set(lumina_FOUND YES)

include(CMakeFindDependencyMacro)
find_dependency(fmt)

if(lumina_FOUND)
  include("${CMAKE_CURRENT_LIST_DIR}/luminaTargets.cmake")
endif()
