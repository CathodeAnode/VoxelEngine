#ifndef SCENE_H
#define SCENE_H

#include <glm/glm.hpp>
#include <iostream>
#include <chrono>

#include "voxel_renderer.h"
#include "voxel_mesher.h"


class Camera;
template<typename ChunkType> class World;

template<typename ChunkType>
class Scene
{
public:
	Scene(Camera& camera, World<ChunkType>& world, VoxelRenderer& voxelRenderer); // add DI for models/character container in the future

	void Update();
	void Render();

	inline void SwitchCamera(Camera& camera) { m_Camera = camera; }
	inline void SetRenderDistance(unsigned int renderDist) { m_RenderDistance = renderDist; }

private:
	VoxelMesher<ChunkType> m_Mesher;
	Camera& m_Camera;
	World<ChunkType>& m_World;
	VoxelRenderer& m_VoxelRenderer;

	unsigned int m_RenderDistance = 10;
};



template<typename ChunkType>
Scene<ChunkType>::Scene(Camera& camera, World<ChunkType>& world, VoxelRenderer& voxelRenderer)
	: m_Camera(camera)
	, m_World(world)
	, m_VoxelRenderer(voxelRenderer)
{}

template<typename ChunkType>
void Scene<ChunkType>::Update()
{
    m_Camera.Update();

    const Frustum& camFrustum = m_Camera.GetFrustum();
    const int chunkSize = ChunkType::Size;

    for (float forwardIncrement = 0; forwardIncrement < m_RenderDistance; ++forwardIncrement)
    {
        for (float rightIncrement = 0; rightIncrement < m_RenderDistance; ++rightIncrement)
        {
            for (float upIncrement = 0; upIncrement < m_RenderDistance; ++upIncrement)
            {
                glm::ivec3 chunkPos = m_Camera.pos - ((m_Camera.right + m_Camera.up) * (float)(m_RenderDistance / 2))
                    + (m_Camera.front * forwardIncrement + m_Camera.right * rightIncrement + m_Camera.up * upIncrement);

                VoxelObjectID chunkID = m_World.GetChunkID(chunkPos);

                if (chunkID == NULL) continue;

                glm::ivec3 chunkMaxPoint = chunkPos + chunkSize;

                if (!camFrustum.isAABBInFrustum(chunkPos, chunkMaxPoint)) continue;


                if (!m_VoxelRenderer.IsCached(chunkID))
                {
                    m_VoxelRenderer.Upload(m_World.getGrid(), chunkPos, m_Mesher);
                }

                glm::ivec3 chunkWorldPos = chunkPos * chunkSize;
                m_VoxelRenderer.DrawOnNextFrame(chunkID, chunkWorldPos);
            }
        }
    }
}


template<typename ChunkType>
void Scene<ChunkType>::Render()
{
	m_VoxelRenderer.Render();
	m_VoxelRenderer.NextFrame();
}


typedef Scene<Chunk8> Scene8;
typedef Scene<Chunk16>Scene16;
typedef Scene<Chunk32>Scene32;


#endif

