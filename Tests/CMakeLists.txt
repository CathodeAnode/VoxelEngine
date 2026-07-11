# ============================================
# Voxel Engine Tests
# ============================================

# Only build tests if enabled
if(NOT VE_BUILD_TEST)
    return()
endif()


# Create test executable
add_executable(VoxelEngineTests)
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
target_sources(VoxelEngineTests PRIVATE ${TEST_SOURCES})

target_link_libraries(VoxelEngineTests PRIVATE VoxelEngineCore GTest::gtest GTest::gtest_main)

# Include test helpers
target_include_directories(VoxelEngineTests PRIVATE
    ${CMAKE_SOURCE_DIR}/VoxelEngine
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Helpers
)

# Enable automatic test discovery
include(GoogleTest)
gtest_discover_tests(VoxelEngineTests)
