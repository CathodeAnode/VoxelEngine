#include "debugging_renderer.h"

DebuggingRenderer::DebuggingRenderer()
	: m_DrawCameraFrustum(false)
	, m_DrawChunkBounds(false)
	, m_DrawMode(DrawMode::Fill)
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

}

void DebuggingRenderer::_RenderChunkBounds(const Camera& mainCamera)
{
}

