# ============================================
# Dependency Configuration for Voxel Engine
# ============================================

set(VE_THIRD_PARTY_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Linking/include)
set(VE_THIRD_PARTY_LIB_DIR     ${CMAKE_SOURCE_DIR}/Linking/lib)

# --------------------------------------------
# GLFW (precompiled .a)
# --------------------------------------------
if(WIN32)
    add_library(glfw STATIC IMPORTED)

    set_target_properties(glfw PROPERTIES
        IMPORTED_LOCATION "${VE_THIRD_PARTY_LIB_DIR}/GLFW/glfw3.a"
        INTERFACE_INCLUDE_DIRECTORIES "${VE_THIRD_PARTY_INCLUDE_DIR}"
    )
endif()

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
# ImGui (compiled from source)
# --------------------------------------------
file(GLOB IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/Linking/src/imgui/*.cpp
    ${CMAKE_SOURCE_DIR}/Linking/src/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/Linking/src/imgui/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC
    ${VE_THIRD_PARTY_INCLUDE_DIR}/imgui
    ${VE_THIRD_PARTY_INCLUDE_DIR}/imgui/backends
)

target_link_libraries(imgui PUBLIC
    glfw
    glad
)


# --------------------------------------------
# GoogleTest (only if building tests)
# --------------------------------------------
if(VE_BUILD_TEST)
    include(CTest)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    # Prevent GoogleTest from forcing /MD on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)
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