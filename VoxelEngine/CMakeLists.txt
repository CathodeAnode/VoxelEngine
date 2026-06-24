# ============================================
# VoxelEngine Executable
# ============================================

# Collect all source files
file(GLOB_RECURSE VE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

# Create the executable
add_executable(VoxelEngineExe ${VE_SOURCES})

# Include directory (your folder is flat)
target_include_directories(VoxelEngineExe
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(VoxelEngineExe
    PUBLIC
        glm
        stb
        glad
        spdlog
        glfw
        ${VE_SYSTEM_LIBS}
)

# Apply compile definitions (from ConfigDefinition.cmake)
target_compile_definitions(VoxelEngineExe PUBLIC
    $<$<BOOL:${VE_LOGGING_ENABLED}>:VE_LOGGING_ENABLED>
    $<$<BOOL:${VE_PROFILING_ENABLED}>:VE_PROFILING_ENABLED>
    $<$<STREQUAL:${VE_CHUNK_TYPE},CHUNK8>:VE_CHUNK_TYPE_8>
    $<$<STREQUAL:${VE_CHUNK_TYPE},CHUNK16>:VE_CHUNK_TYPE_16>
    $<$<STREQUAL:${VE_CHUNK_TYPE},CHUNK32>:VE_CHUNK_TYPE_32>
)

# Optional: warnings
if(MSVC)
    target_compile_options(VoxelEngineExe PRIVATE /W4 /permissive-)
else()
    target_compile_options(VoxelEngineExe PRIVATE -Wall -Wextra -Wpedantic)
endif()