#ifndef SCENE_H
#define SCENE_H

#include <glm/glm.hpp>

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

	unsigned int m_RenderDistance = 32;
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
	// call world update in the future (for chunk data compression and managment)

	// TODO use SIMD to acclerate frustum culling
	// do frustum culling
	const Frustum& camFrustum = m_Camera.GetFrustum();
	for (float forwardIncrement = 0; forwardIncrement < m_RenderDistance; ++forwardIncrement)
	{
		for (float rightIncrement = 0; rightIncrement < m_RenderDistance; ++rightIncrement)
		{
			for (float upIncrement = 0; upIncrement < m_RenderDistance; ++upIncrement)
			{
				glm::ivec3 step = m_Camera.front * forwardIncrement + m_Camera.right * rightIncrement + m_Camera.up * upIncrement;

				//do camera + step & shift stepping by renderdist/2 to left and down
				//calculate AABB of chunk
				//check if that chunk's AABB is in frustum
				// => if not in frustum: continue to next iteration
				// request object ID of chunk coords from world
				// => if chunk does not exists (world returns object ID  = 0): continue to next iteration
				// check if chunk mesh is cached on gpu
				// => if chunk is not cached: get chunk data from world & mesh chunk
				// draw chunk on next frame
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

