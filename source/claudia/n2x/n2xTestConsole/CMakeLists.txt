cmake_minimum_required(VERSION 3.10)

project(n2xTestConsole)

add_executable(n2xTestConsole)

set(SOURCES
	n2xTestConsole.cpp
)

target_sources(n2xTestConsole PRIVATE ${SOURCES})
source_group("source" FILES ${SOURCES})

target_link_libraries(n2xTestConsole PUBLIC n2xLib)
set_property(TARGET n2xTestConsole PROPERTY FOLDER "N2x")

# Opt-in Overflash comparison executable; never built in default builds.
option(GEARMULATOR_BUILD_OVERFLASH_N2X_REFERENCE "Build Nord Lead 2X offline comparison fixture" OFF)
if(GEARMULATOR_BUILD_OVERFLASH_N2X_REFERENCE)
    add_executable(n2xOverflashReference n2xOverflashReference.cpp)
    target_link_libraries(n2xOverflashReference PRIVATE n2xLib)
    set_property(TARGET n2xOverflashReference PROPERTY FOLDER "N2x")
endif()

# Explicit opt-in shared C ABI. Not part of normal Gearmulator builds.
option(GEARMULATOR_BUILD_OVERFLASH_N2X_BRIDGE "Build Nord Lead 2X Overflash C ABI shared library" OFF)
if(GEARMULATOR_BUILD_OVERFLASH_N2X_BRIDGE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_library(n2xOverflashBridge SHARED n2xOverflashBridge.cpp)
    target_link_libraries(n2xOverflashBridge PRIVATE n2xLib)
    target_compile_features(n2xOverflashBridge PRIVATE cxx_std_17)
    set_property(TARGET n2xOverflashBridge PROPERTY FOLDER "N2x")
endif()
