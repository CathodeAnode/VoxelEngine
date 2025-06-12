#ifndef WORLD_H
#define WORLD_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glfw/glfw3.h>

#include <unordered_map>

#include "chunk.h"

class World {
public:

	World(unsigned int _renderDistance, glm::vec3* _playerCoords);
	~World();

	/**
	 * Renders all chunks within a cubic area defined by renderDistance^3
	 * (renderDistance x renderDistance x renderDistance) centered around the player.
	 */
	void basicRender();
	/**
	 * Renders only the chunks currently within the camera's view frustum
	 */
	void FrustumRender();

private:
	unsigned int renderDistance;
	Chunk* chunksRendered;
	std::unordered_map<int, Chunk> dirtyChunks; // Maps 1D coordinate of chunk to edited chunk data
	glm::vec3* playerCoords;
};

#endif
