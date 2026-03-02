#ifndef DEBUGGING_RENDERER_H
#define DEBUGGING_RENDERER_H

#include<glad/glad.h>
#include<glm/fwd.hpp>

#include "camera.h"
#include "shader.h"

//TODO: make this not compile in distribution mode

enum class DrawMode : std::uint8_t
{
    Fill = 0,
    Wireframe = 1
};

class DebuggingRenderer
{
public:
    explicit DebuggingRenderer(Camera& mainCamera);

    void Init();

    void RenderOverlay();

    Camera& GetDebugCamera() { return m_DebugCamera; }

    // Visualization toggles
    void SetFrustumOutline(bool enabled) { m_DrawCameraFrustum = enabled; }
    void SetDrawMode(DrawMode mode) { m_DrawMode = mode; }
    void SetChunkBounds(bool enabled) { m_DrawChunkBounds = enabled; }
    void SetCameraGizmo(bool enabled) { m_DrawCameraGizmo = enabled; }

    void ToggleFrustumOutline() { m_DrawCameraFrustum = !m_DrawCameraFrustum; }
    void ToggleChunkBounds() { m_DrawChunkBounds = !m_DrawChunkBounds; }
    void ToggleCameraGizmo() { m_DrawCameraGizmo = !m_DrawCameraGizmo; }

private:
    void _ApplyPolygonMode();
    void _RenderFrustum() {};
    void _RenderCameraGizmo() {};
    void _RenderChunkBounds() {};

private:
    Camera& m_MainCamera;
    Camera  m_DebugCamera;

    Shader m_FrustumOutlineShader;
    unsigned int m_LineVAO;
    unsigned int m_LineVBO;

    DrawMode m_DrawMode;
    bool m_DrawCameraFrustum;
    bool m_DrawChunkBounds;
    bool m_DrawCameraGizmo;
};



#endif
