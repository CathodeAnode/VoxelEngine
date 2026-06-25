# =============================
# Define Project-Wide Settings
# =============================
cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

# Enables the Visibility Property on all target types
cmake_policy(SET CMP0063 NEW)
# Enables the MSVC_RUNTIME_LIBRARY property on targets
cmake_policy(SET CMP0091 NEW)

# Define the Project Name and Description
project(VoxelEngine
    LANGUAGES CXX C # C language is used to compile glad
    DESCRIPTION "GPU-Driven voxel engine using OpenGL 4.6"
)

# Set Project version (used for library versioning and for CPack)
set(CMAKE_PROJECT_VERSION_MAJOR 0)
set(CMAKE_PROJECT_VERSION_MINOR 1)
set(CMAKE_PROJECT_VERSION_PATCH 1)
set(CMAKE_PROJECT_VERSION ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH})

# Set required C++ Standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Generate the compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Set the Relative Path Configurations
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Configure the visibility of symbols in targets
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# =============================
# Define CMake Module Imports
# =============================
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(${CMAKE_SOURCE_DIR}/CMake/ConfigOptions.cmake)
include(${CMAKE_SOURCE_DIR}/CMake/ConfigDefinitions.cmake)
include(${CMAKE_SOURCE_DIR}/CMake/ConfigDependencies.cmake)

# =============================
# Define Targets
# =============================
add_subdirectory(VoxelEngine)
add_subdirectory(Tests)

