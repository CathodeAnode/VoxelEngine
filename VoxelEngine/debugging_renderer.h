#ifndef DEBUGGING_RENDERER_H
#define DEBUGGING_RENDERER_H

#include<glad/glad.h>
#include<glm/fwd.hpp>

//TODO: make this not compile in distribution mode

class Camera;
class CameraManager;

enum class DrawMode : std::uint8_t
{
    Fill = 0,
    Wireframe = 1
};

class DebuggingRenderer
{
public:
    explicit DebuggingRenderer();

    void RenderOverlay(const Camera& camera);

    // Visualization toggles
    void SetFrustumOutline(bool enabled) { m_DrawCameraFrustum = enabled; }
    void SetDrawMode(DrawMode mode) { m_DrawMode = mode; }
    void SetChunkBounds(bool enabled) { m_DrawChunkBounds = enabled; }

    void ToggleFrustumOutline() { m_DrawCameraFrustum = !m_DrawCameraFrustum; }
    void ToggleChunkBounds() { m_DrawChunkBounds = !m_DrawChunkBounds; }

    DrawMode GetDrawMode() const { return m_DrawMode; }

private:
    void _ApplyPolygonMode();
    void _RenderCameraFrustum(const Camera& mainCamera);
    void _RenderChunkBounds(const Camera& mainCamera);

private:
    DrawMode m_DrawMode;
    bool m_DrawCameraFrustum;
    bool m_DrawChunkBounds;
};



#endif
