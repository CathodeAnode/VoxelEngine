# ============================================
# Dependency Configuration for Voxel Engine
# ============================================

set(VE_THIRD_PARTY_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Linking/include)
set(VE_THIRD_PARTY_LIB_DIR     ${CMAKE_SOURCE_DIR}/Linking/lib)

# --------------------------------------------
# GLFW (precompiled .a)
# --------------------------------------------
add_library(glfw STATIC IMPORTED)

set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION "${VE_THIRD_PARTY_LIB_DIR}/GLFW/glfw3.a"
    INTERFACE_INCLUDE_DIRECTORIES "${VE_THIRD_PARTY_INCLUDE_DIR}"
)

# --------------------------------------------
# GLM (header-only)
# --------------------------------------------
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE
    ${VE_THIRD_PARTY_INCLUDE_DIR}
)

# --------------------------------------------
# SPDLOG (header-only)
# --------------------------------------------
add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE
    ${VE_THIRD_PARTY_INCLUDE_DIR}
)

# --------------------------------------------
# STB (header-only)
# --------------------------------------------
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE
    ${VE_THIRD_PARTY_INCLUDE_DIR}
)

# --------------------------------------------
# GLAD (header-only)
# --------------------------------------------
add_library(glad STATIC
    ${CMAKE_SOURCE_DIR}/VoxelEngine/glad.c
)

set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)
target_compile_options(glad PRIVATE
    -Wno-cast-function-type
    -Wno-pedantic
)

target_include_directories(glad PUBLIC
    ${VE_THIRD_PARTY_INCLUDE_DIR}
)


# --------------------------------------------
# GoogleTest (only if building tests)
# --------------------------------------------
if(VE_BUILD_TEST)
    include(CTest)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)

    # GTest::gtest and GTest::gtest_main will now be available
endif()


# --------------------------------------------
# Windows system libraries
# --------------------------------------------
set(VE_SYSTEM_LIBS
    opengl32
    user32
    gdi32
    shell32
)