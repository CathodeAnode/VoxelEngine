#include "debugging_renderer.h"

#include "gizmos.h"
#include "voxel_math.h"
#include "camera.h"

DebuggingRenderer::DebuggingRenderer(unsigned int chunkSize)
    : k_ChunkSize(chunkSize)
	, m_DrawMode(DrawMode::Fill)
	, m_DrawCameraFrustum(false)
	, m_DrawChunkBounds(false)
{}

void DebuggingRenderer::RenderOverlay(const Camera& mainCamera)
{
	_ApplyPolygonMode();

	if (m_DrawCameraFrustum)
		_RenderCameraFrustum(mainCamera);

	if (m_DrawChunkBounds)
		_RenderChunkBounds(mainCamera);
}

void DebuggingRenderer::_ApplyPolygonMode()
{
	if (m_DrawMode == DrawMode::Wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void DebuggingRenderer::_RenderCameraFrustum(const Camera& mainCamera)
{
	Gizmos::DrawFrustum(mainCamera);
}

void DebuggingRenderer::_RenderChunkBounds(const Camera& mainCamera)
{
    constexpr int GRID_SIZE = 5;
    constexpr int HALF = GRID_SIZE / 2;

    glm::ivec3 camChunk = WorldToChunk(mainCamera.pos, k_ChunkSize);

    glm::vec3 start = glm::vec3(camChunk - glm::ivec3(HALF)) * (float)k_ChunkSize;
    float extent = GRID_SIZE * k_ChunkSize;

    // Lines along X
    for (int y = 0; y <= GRID_SIZE; ++y)
    {
        for (int z = 0; z <= GRID_SIZE; ++z)
        {
            glm::vec3 a = start + glm::vec3(0, y * k_ChunkSize, z * k_ChunkSize);
            glm::vec3 b = a + glm::vec3(extent, 0, 0);
            Gizmos::DrawLine(a, b);
        }
    }

    // Lines along Y
    for (int x = 0; x <= GRID_SIZE; ++x)
    {
        for (int z = 0; z <= GRID_SIZE; ++z)
        {
            glm::vec3 a = start + glm::vec3(x * k_ChunkSize, 0, z * k_ChunkSize);
            glm::vec3 b = a + glm::vec3(0, extent, 0);
            Gizmos::DrawLine(a, b);
        }
    }

    // Lines along Z
    for (int x = 0; x <= GRID_SIZE; ++x)
    {
        for (int y = 0; y <= GRID_SIZE; ++y)
        {
            glm::vec3 a = start + glm::vec3(x * k_ChunkSize, y * k_ChunkSize, 0);
            glm::vec3 b = a + glm::vec3(0, 0, extent);
            Gizmos::DrawLine(a, b);
        }
    }
}

