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
	Scene(Camera& camera, ChunkManager<ChunkType>& world, VoxelRenderer<ChunkType>& renderer); // add DI for models/character container in the future

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void Update();
	void Render();

	void SetWorldUpdate(bool enabled) { m_WorldUpdateFlag = enabled; }

	inline void SwitchCamera(Camera& camera) { m_Camera = camera; }

private:
    inline void _UploadTerrain();

private:
	Camera& m_Camera;
	ChunkManager<ChunkType>& m_World;
	VoxelRenderer<ChunkType>& m_Renderer;

	bool m_DirtyFrame;
	bool m_WorldUpdateFlag;

};



typedef Scene<Chunk8> Scene8;
typedef Scene<Chunk16>Scene16;
typedef Scene<Chunk32>Scene32;


#include "scene.tpp"

#endif

