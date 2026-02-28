#include "debugging_renderer.h"

DebuggingRenderer::DebuggingRenderer(Camera& mainCamera)
	: m_MainCamera(mainCamera)
	, m_DebugCamera(mainCamera)
{}

void DebuggingRenderer::Init()
{
	// TODO: m_DebugCamera
	// - position Debug Camera orthogonally to main camera
	// - look at main camera
	// - set z far to be able to see main camera from a distance

	m_FrustumOutlineShader = Shader({
	{ "frustum_outline.vert.glsl", GL_VERTEX_SHADER },
	{ "frustum_outline.frag.glsl", GL_FRAGMENT_SHADER }
		});

	glGenVertexArrays(1, &m_LineVAO);
}

void DebuggingRenderer::RenderOverlay()
{
	_ApplyPolygonMode();

	if (m_DrawCameraGizmo)
		_RenderCameraGizmo();

	if (m_DrawCameraFrustum)
		_RenderFrustum();

	if (m_DrawChunkBounds)
		_RenderChunkBounds();
}

void DebuggingRenderer::_ApplyPolygonMode()
{
	if (m_DrawMode == DrawMode::Wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

