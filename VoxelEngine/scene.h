#ifndef SCENE_H
#define SCENE_H

#include <glm/glm.hpp>
#include <iostream>
#include <chrono>

class Camera;

template<typename ChunkType>
class Terrian;

template<typename ChunkType>
class VoxelRenderer;

template<typename ChunkType>
class Scene
{
public:
	Scene(Camera& camera, Terrian<ChunkType>& world, VoxelRenderer<ChunkType>& renderer); // add DI for models/character container in the future

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void Update();
	void Render();

	inline void SwitchCamera(Camera& camera) { m_Camera = camera; }

private:
    inline void _UploadTerrian();

private:
	Camera& m_Camera;
	Terrian<ChunkType>& m_World;
	VoxelRenderer<ChunkType>& m_Renderer;

	bool m_DirtyFrame;

};



typedef Scene<Chunk8> Scene8;
typedef Scene<Chunk16>Scene16;
typedef Scene<Chunk32>Scene32;


#include "scene.tpp"

#endif

