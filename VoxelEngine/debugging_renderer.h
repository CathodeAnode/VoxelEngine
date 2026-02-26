#ifndef DEBUGGING_RENDERER_H
#define DEBUGGING_RENDERER_H

#include<glad/glad.h>
#include<glm/fwd.hpp>

#include "camera.h"

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

    void Update(float deltaTime);

    void RenderOverlay();

    Camera& GetDebugCamera() { return m_DebugCamera; }
    bool IsDebugViewEnabled() const { return m_DebugViewEnabled; }
    void EnableDebugView(bool enabled) { m_DebugViewEnabled = enabled; }

    // Visualization toggles
    void SetFrustumOutline(bool enabled) { m_ShowFrustum = enabled; }
    void SetDrawMode(DrawMode mode) { m_DrawMode = mode; }
    void SetChunkBounds(bool enabled) { m_ShowChunkBounds = enabled; }
    void SetCameraGizmo(bool enabled) { m_ShowCameraGizmo = enabled; }

    void ToggleFrustumOutline() { m_ShowFrustum = !m_ShowFrustum; }
    void ToggleChunkBounds() { m_ShowChunkBounds = !m_ShowChunkBounds; }
    void ToggleCameraGizmo() { m_ShowCameraGizmo = !m_ShowCameraGizmo; }

private:
    void _ApplyPolygonMode();
    void _RenderFrustum();

private:
    Camera& m_MainCamera;
    Camera  m_DebugCamera;

    bool m_DebugViewEnabled = false;
    bool m_ShowFrustum = false;
    bool m_ShowChunkBounds = false;
    bool m_ShowCameraGizmo = false;

    DrawMode m_DrawMode = DrawMode::Fill;

    // Debug geometry buffers
    unsigned int m_LineVAO = 0;
    unsigned int m_LineVBO = 0;
};



#endif
