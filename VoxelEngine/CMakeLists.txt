# ============================================
# VoxelEngine Executable
# ============================================

# Collect all source files
add_library(VoxelEngineCore STATIC)   # everything except main.cpp
ve_add_module_sources(VoxelEngineCore ${CMAKE_CURRENT_SOURCE_DIR}
    EXCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
target_link_libraries(VoxelEngineCore PUBLIC glfw glad glm spdlog stb imgui)

add_executable(VoxelEngine main.cpp)
target_link_libraries(VoxelEngine PRIVATE VoxelEngineCore)

target_include_directories(VoxelEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Collect all asset files recursively
file(GLOB_RECURSE VE_ASSETS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
)

foreach(asset ${VE_ASSETS})
    file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}" "${asset}")
    get_filename_component(rel_dir "${rel_path}" DIRECTORY)

    add_custom_command(
        TARGET VoxelEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_CURRENT_BINARY_DIR}/${rel_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${asset}"
                "${CMAKE_CURRENT_BINARY_DIR}/${rel_dir}"
    )
endforeach()

# Apply compile definitions (from ConfigDefinition.cmake)
target_compile_definitions(VoxelEngine PUBLIC
    $<$<BOOL:${VE_LOGGING_ENABLED}>:VE_LOGGING_ENABLED>
    $<$<BOOL:${VE_PROFILING_ENABLED}>:VE_PROFILING_ENABLED>
    $<$<STREQUAL:${VE_CHUNK_TYPE},CHUNK8>:VE_CHUNK_TYPE_8>
    $<$<STREQUAL:${VE_CHUNK_TYPE},CHUNK16>:VE_CHUNK_TYPE_16>
    $<$<STREQUAL:${VE_CHUNK_TYPE},CHUNK32>:VE_CHUNK_TYPE_32>
)

# Optional: warnings
if(MSVC)
    target_compile_options(VoxelEngine PRIVATE /W4 /permissive-)
else()
    target_compile_options(VoxelEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()