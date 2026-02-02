if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/lumina-${PROJECT_VERSION}"
      CACHE STRING ""
  )
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
# should match the name of variable set in the install-config.cmake script
set(package lumina)

install(
    DIRECTORY
    include/
    "${PROJECT_BINARY_DIR}/export/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT lumina_Development
)

install(
    TARGETS lumina_lumina imgui linalg
    EXPORT luminaTargets
    RUNTIME #
    COMPONENT lumina_Runtime
    LIBRARY #
    COMPONENT lumina_Runtime
    NAMELINK_COMPONENT lumina_Development
    ARCHIVE #
    COMPONENT lumina_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    lumina_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE STRING "CMake package config location relative to the install prefix"
)
set_property(CACHE lumina_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
mark_as_advanced(lumina_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${lumina_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT lumina_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${lumina_INSTALL_CMAKEDIR}"
    COMPONENT lumina_Development
)

install(
    EXPORT luminaTargets
    NAMESPACE lumina::
    DESTINATION "${lumina_INSTALL_CMAKEDIR}"
    COMPONENT lumina_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
