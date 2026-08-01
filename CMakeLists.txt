cmake_minimum_required(VERSION 3.20)

project(
    GalacticShrine.GsId
    VERSION 1.0.0
    DESCRIPTION "Système d'identifiants Galactic-Shrine de 256 bits pour C++"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(GSID_BUILD_TESTS "Construire les tests GsId" ON)
option(GSID_BUILD_EXAMPLES "Construire les exemples GsId" ON)

add_library(GalacticShrine.GsId src/gsid.cpp)
add_library(GalacticShrine::GsId ALIAS GalacticShrine.GsId)

target_compile_features(GalacticShrine.GsId PUBLIC cxx_std_20)

target_include_directories(
    GalacticShrine.GsId
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(
    GalacticShrine.GsId
    PROPERTIES
        CXX_EXTENSIONS OFF
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        EXPORT_NAME GsId
)

if(MSVC)
    target_compile_options(GalacticShrine.GsId PRIVATE /W4 /permissive-)
    target_link_libraries(GalacticShrine.GsId PRIVATE bcrypt)
else()
    target_compile_options(GalacticShrine.GsId PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(APPLE)
    target_link_libraries(GalacticShrine.GsId PRIVATE "-framework Security")
endif()

if(GSID_BUILD_TESTS)
    enable_testing()

    add_executable(gsid_tests tests/test_gsid.cpp)
    target_link_libraries(gsid_tests PRIVATE GalacticShrine::GsId)
    target_compile_features(gsid_tests PRIVATE cxx_std_20)

    add_test(NAME GalacticShrine.GsId.Tests COMMAND gsid_tests)
endif()

if(GSID_BUILD_EXAMPLES)
    add_executable(gsid_example examples/basic.cpp)
    target_link_libraries(gsid_example PRIVATE GalacticShrine::GsId)
    target_compile_features(gsid_example PRIVATE cxx_std_20)
endif()

install(
    TARGETS GalacticShrine.GsId
    EXPORT GalacticShrineGsIdTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

configure_package_config_file(
    cmake/GalacticShrineGsIdConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/GalacticShrineGsIdConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GalacticShrineGsId
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/GalacticShrineGsIdConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/GalacticShrineGsIdConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/GalacticShrineGsIdConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GalacticShrineGsId
)

install(
    EXPORT GalacticShrineGsIdTargets
    FILE GalacticShrineGsIdTargets.cmake
    NAMESPACE GalacticShrine::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GalacticShrineGsId
)
