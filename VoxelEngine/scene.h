#ifndef SCENE_H
#define SCENE_H

#include <glm/glm.hpp>
#include <iostream>
#include <chrono>

#include "chunk.h"
#include "voxel_math.h"

class Camera;

template<typename ChunkType>
class ChunkManager;

template<typename ChunkType>
class VoxelRenderer;

template<typename ChunkType>
class Scene
{
public:
	Scene(ChunkManager<ChunkType>& world, VoxelRenderer<ChunkType>& renderer); // add DI for models/character container in the future
	
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void Init(unsigned int renderDistance, const glm::vec3& startingCameraPos);
	void Update(const glm::vec3& cameraPos);
	void Render(const Camera& viewCamera, const Camera& cullCamera);

private:
    inline void _UploadLoadedTerrain(const glm::vec3& cameraPos);

private:
	ChunkManager<ChunkType>& m_World;
	VoxelRenderer<ChunkType>& m_Renderer;

	unsigned int m_RenderDist = 0;

};



typedef Scene<Chunk8> Scene8;
typedef Scene<Chunk16>Scene16;
typedef Scene<Chunk32>Scene32;


#include "scene.tpp"

#endif

