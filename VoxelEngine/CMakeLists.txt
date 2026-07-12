# ============================================
# VoxelEngine Executable
# ============================================

# Collect all source files
add_library(VoxelEngineCore STATIC)   # everything except main.cpp
ve_add_module_sources(VoxelEngineCore ${CMAKE_CURRENT_SOURCE_DIR}
    EXCLUDE 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp

    # TEMP exclude non-module cpp files, until migration is complete
    ${CMAKE_CURRENT_SOURCE_DIR}/camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compute_shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/debug_ui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/debugging_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/frame_counter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gizmos.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gpu_buffer_lock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/perlin_noise.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/profiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/screen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_image_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/uid_manager.cpp

)

# TEMP target_source non-module cpp files
file(GLOB TEMP_CPP_FILES  CONFIGURE_DEPENDS 
    ${CMAKE_CURRENT_SOURCE_DIR}/camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compute_shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/debug_ui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/debugging_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/frame_counter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gizmos.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gpu_buffer_lock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/perlin_noise.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/profiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/screen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_image_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/uid_manager.cpp
)
target_sources(VoxelEngineCore PRIVATE ${TEMP_CPP_FILES})

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