# Logging
if(VE_LOGGING_ENABLED)
    add_compile_definitions(VE_LOGGING_ENABLED)
endif()

# Profiling
if(VE_PROFILING_ENABLED)
    add_compile_definitions(VE_PROFILING_ENABLED)
endif()

# Chunk type
if(VE_CHUNK_TYPE STREQUAL "CHUNK8")
    add_compile_definitions(VE_CHUNK_TYPE_8)
elseif(VE_CHUNK_TYPE STREQUAL "CHUNK16")
    add_compile_definitions(VE_CHUNK_TYPE_16)
elseif(VE_CHUNK_TYPE STREQUAL "CHUNK32")
    add_compile_definitions(VE_CHUNK_TYPE_32)
else()
    message(FATAL_ERROR "Invalid VE_CHUNK_TYPE: ${VE_CHUNK_TYPE}")
endif()