# ============================================
# Voxel Engine Tests
# ============================================

# Only build tests if enabled
if(NOT VE_BUILD_TEST)
    return()
endif()

# Collect all test sources
file(GLOB_RECURSE VE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# TEMP (Until VoxelEngine is built as dll instead)
set(VE_ENGINE_SOURCES
    ${CMAKE_SOURCE_DIR}/VoxelEngine/gpu_buffer_lock.cpp
    ${CMAKE_SOURCE_DIR}/VoxelEngine/joystick.cpp
    ${CMAKE_SOURCE_DIR}/VoxelEngine/keyboard.cpp
    ${CMAKE_SOURCE_DIR}/VoxelEngine/logger.cpp
    ${CMAKE_SOURCE_DIR}/VoxelEngine/mouse.cpp
    ${CMAKE_SOURCE_DIR}/VoxelEngine/profiler.cpp
    ${CMAKE_SOURCE_DIR}/VoxelEngine/screen.cpp
    ${CMAKE_SOURCE_DIR}/VoxelEngine/shader.cpp
)

# Create test executable
add_executable(VoxelEngineTests ${VE_TEST_SOURCES} ${VE_ENGINE_SOURCES})

# Include test helpers
target_include_directories(VoxelEngineTests PRIVATE
    ${CMAKE_SOURCE_DIR}/VoxelEngine
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Helpers
)

# Link engine + GTest
target_link_libraries(VoxelEngineTests PRIVATE
    glm
    stb
    glad
    spdlog
    glfw
    imgui
    ${VE_SYSTEM_LIBS}
    GTest::gtest
    GTest::gtest_main
)

# Enable automatic test discovery
include(GoogleTest)
gtest_discover_tests(VoxelEngineTests)
