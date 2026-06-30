# =============================
# Base Options
# =============================
option(VE_BUILD_TEST "Build Voxel Engine tests" FALSE)

# =============================
# Compute defaults for logging/profiling
# =============================
set(_DEFAULT_LOGGING TRUE)
set(_DEFAULT_PROFILING TRUE)

# =============================
# Logging & Profiling Options
# =============================
option(VE_LOGGING_ENABLED "Enable Voxel Engine logging" ${_DEFAULT_LOGGING})
option(VE_PROFILING_ENABLED "Enable Voxel Engine profiling" ${_DEFAULT_PROFILING})

# =============================
# Chunk Type Selection
# =============================
set(VE_CHUNK_TYPE "CHUNK8" CACHE STRING "Voxel Engine chunk type")
set_property(CACHE VE_CHUNK_TYPE PROPERTY STRINGS CHUNK8 CHUNK16 CHUNK32)