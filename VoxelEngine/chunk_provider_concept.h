#ifndef INTERFACE_CHUNK_CONTAINER_H
#define INTERFACE_CHUNK_CONTAINER_H

#include <memory>
#include <type_traits>

// Forward declare glm::ivec3
#include <glm/fwd.hpp> 

template <typename T, typename ChunkType>
concept ChunkProvider =
    requires(T t, const glm::ivec3 & coords) {
        { t.GetChunk(coords) } -> std::same_as<std::shared_ptr<ChunkType>>;
}
&&
    requires(const T t, const glm::ivec3& coords) {
        { t.GetChunk(coords) } -> std::same_as<std::shared_ptr<const ChunkType>>;
}
&&
    requires(const T t) {
        { t.GetUID() } -> std::same_as<VoxelObjectID>;
};

#endif