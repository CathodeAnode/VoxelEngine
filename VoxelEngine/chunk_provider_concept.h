#ifndef INTERFACE_CHUNK_CONTAINER_H
#define INTERFACE_CHUNK_CONTAINER_H

#include <memory>
#include <type_traits>

// Forward declare glm::ivec3
#include <glm/fwd.hpp> 

template <typename T, typename ChunkType>
concept ChunkProvider = requires(T t, const glm::ivec3 & chunkCoords) {
    { t.GetChunk(chunkCoords) } -> std::same_as<std::shared_ptr<ChunkType>>;
    { t.GetChunk(chunkCoords) } -> std::same_as<std::shared_ptr<const ChunkType>>;
};

#endif