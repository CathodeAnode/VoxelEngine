#ifndef VE_APPLICATION_H_
#define VE_APPLICATION_H_

#include <memory>
#include <chrono>

#include "screen.h"
#include "camera.h"
#include "camera_manager.h"
#include "joystick.h"
#include "scene.h"
#include "chunk_manager.h"
#include "voxel_renderer.h"
#include "debugging_renderer.h"
#include "voxel_edit.h"
#include "application_config.h"

template<typename ChunkT>
class Application
{
public:
    using ChunkManagerT = ChunkManager<ChunkT>;
    using MesherT = VoxelMesher<ChunkT>;
    using RendererT = VoxelRenderer<ChunkT>;
    using VoxelEditT = VoxelEdit<ChunkT, ChunkManagerT>;
    using SceneT = Scene<ChunkT>;

public:
    Application(const ApplicationConfig& cfg, std::unique_ptr<ChunkGeneratorStrategy<ChunkT>> generator);
    ~Application();

    bool Init(const ApplicationConfig& cfg);
    void Run();
    void Shutdown();

private:
    void ProcessInput();
    void Update();
    void Render();
    void CalcFPSOnWindowTitle(int avgOverNFrames);

private:
    // Timing
    float m_LastFrameTime = 0.0;
    float m_DeltaTime = 0.0;
    float  m_FPS = 0.0f;
    int    m_FPSLimit = 60.0f;

    // FPS display helpers
    int   m_CountFPS = 0;
    float m_SumFPS = 0.0f;

    Screen m_Screen;

    // World / Rendering
    ChunkManagerT     m_World;
    RendererT         m_Renderer;
    VoxelEditT        m_VoxelEdit;
    SceneT            m_Scene;
    DebuggingRenderer m_DebuggingRenderer;

    // Input / Camera
    Joystick m_MainJoystick;
    CameraManager m_CameraManager;
    CameraManager::CameraId m_MainCameraID;
    CameraManager::CameraId m_DebugCameraID;

    // Application variables
    glm::ivec3 m_VoxelAimedAt;
    bool m_AimedRaycastHit;
};

#include "application.tpp"

#endif

