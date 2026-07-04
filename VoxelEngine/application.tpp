#ifndef VOXA_APPLICATION_TPP
#define VOXA_APPLICATION_TPP

#include "application.h"

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "logger.h"
#include "profiler.h"
#include "keyboard.h"
#include "mouse.h"
#include "voxel_ray_cast.h"
#include "chunk_generator_strategy.h"
#include "voxel_mesher.h"
#include "gizmos.h"
#include "frame_counter.h"
#include "debug_ui.h"


template<typename ChunkT>
Application<ChunkT>::Application(const ApplicationConfig& cfg, std::unique_ptr<ChunkGeneratorStrategy<ChunkT>> generator)
    : m_Screen(cfg.ScreenWidth, cfg.ScreenHeight, cfg.Name)
    , m_World(cfg.startingWorldPos, cfg.loadedChunkDistance, std::move(generator))
    , m_Renderer(std::make_unique<MesherT>())
    , m_VoxelEdit(m_World)
    , m_Scene(cfg.ThreadWorkers, m_World, m_Renderer, m_VoxelEdit)
    , m_DebuggingRenderer(ChunkT::Size)
    , m_MainJoystick(0)
{
    LOG_INFO(EngineSystem::VOXEL_ENGINE,
        "Voxel engine configuration: chunk={}x{}x{}",
        ChunkT::Size,
        sizeof(typename ChunkT::ValueType) * 8,
        ChunkT::Size);

    const float camNear = 0.1f;
    const float camFar = 1000.0f;

    std::unique_ptr<Camera> mainCamera = std::make_unique<Camera>(cfg.startingWorldPos, cfg.ScreenWidth, cfg.ScreenHeight, camNear, camFar);

    float renderRadius = cfg.renderChunkDistance * ChunkT::Size;

    // distance needed to see the whole cube of chunks
    const float debugDistance = renderRadius * 1.75f;

    glm::vec3 isoDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
    glm::vec3 debugPos = mainCamera->pos + isoDir * debugDistance;

    std::unique_ptr<Camera> debugCamera = std::make_unique<Camera>(debugPos, cfg.ScreenWidth, cfg.ScreenHeight, 0.1f, debugDistance * 4.0f);

    debugCamera->LookAt(mainCamera->pos);

   m_MainCameraID = m_CameraManager.RegisterCamera(std::move(mainCamera));
   m_DebugCameraID = m_CameraManager.RegisterCamera(std::move(debugCamera));

    m_CameraManager.SetActiveCamera(m_MainCameraID);
}

template<typename ChunkT>
Application<ChunkT>::~Application()
{
    Shutdown();
}

template<typename ChunkT>
bool Application<ChunkT>::Init(const ApplicationConfig& cfg)
{
    if (cfg.loadedChunkDistance > cfg.renderChunkDistance)
    {
        LOG_WARN(
            EngineSystem::VOXEL_ENGINE,
            "Loaded chunk distance ({}) exceeds render distance ({}). Some loaded chunks will not be rendered.",
            cfg.loadedChunkDistance,
            cfg.renderChunkDistance
        );
    }

    if (!m_Screen.init())
    {
        LOG_CRITICAL(EngineSystem::CORE, "Screen initialization failed");
        return false;
    }

    m_Screen.enableInputs();
    m_Screen.toggleCursor();

    m_MainJoystick.Update();
    if (m_MainJoystick.isPresent())
    {
        LOG_INFO(EngineSystem::INPUTS, "Joystick {} connected", m_MainJoystick.getName());
    }
    else
    {
        LOG_INFO(EngineSystem::INPUTS, "Joystick not connected");
    }

    m_Renderer.Init(
        cfg.cacheNumOfPages,
        cfg.cachePageSize,
        static_cast<size_t>(cfg.averageIndirectCmdsPerChunk * pow(cfg.renderChunkDistance, 3))
    );

    DebugUI::Init(m_Screen.getWindow());
    Gizmos::Init();

    m_World.InitializeStartingChunks();
    m_Scene.Init(cfg.renderChunkDistance, m_CameraManager.GetActiveCamera().pos);

    return true;
}

template<typename ChunkT>
void Application<ChunkT>::Run()
{
    Profiler::GetInstance().SetEnabled(false);

    auto lastTime = clock::now();
    m_FPS = 60.0;
    const double smoothing = 0.99; // better for FPS

    while (m_Screen.isOpen())
    {
        PROFILE_SCOPE("Frame");

        auto now = clock::now();
        m_DeltaTime = now - lastTime;
        lastTime = now;

        double frameTime = m_DeltaTime.count(); // seconds
        double currentFPS = 1.0 / frameTime;

        m_FPS = m_FPS * smoothing + currentFPS * (1.0 - smoothing);

        ProcessInput();
        Update();
        Render();

        m_Screen.Flush();
        FrameCounter::Tick();
    }

    Profiler::GetInstance().SetEnabled(true);
}

template<typename ChunkT>
void Application<ChunkT>::ProcessInput() 
{
    PROFILE_FUNCTION();

    Camera& camera = m_CameraManager.GetCamera(m_MainCameraID);

    if (Keyboard::key(Key::Escape))
        m_Screen.close();

    if (Keyboard::keyDown(Key::F1))
    {
        auto& profiler = Profiler::GetInstance();
        profiler.SetEnabled(!profiler.IsEnabled());
    }

    if (Keyboard::keyDown(Key::F2))
    {
        DrawMode newDrawMode = m_DebuggingRenderer.GetDrawMode() == DrawMode::Fill ? DrawMode::Wireframe : DrawMode::Fill;
        m_DebuggingRenderer.SetDrawMode(newDrawMode);
    }

    if (Keyboard::keyDown(Key::F3))
    {
        m_DebuggingRenderer.ToggleChunkBounds();
    }

    if (Keyboard::keyDown(Key::F4))
    {
        bool flip = !DebugUI::IsEnabled();
        DebugUI::Toggle(flip);
    }

    if (Keyboard::keyDown(Key::Tab))
    {
        if (&m_CameraManager.GetActiveCamera() == &m_CameraManager.GetCamera(m_MainCameraID))
        {
            m_CameraManager.SetActiveCamera(m_DebugCameraID);
        }
        else
            m_CameraManager.SetActiveCamera(m_MainCameraID);

        m_DebuggingRenderer.ToggleFrustumOutline();
    }

    if (Keyboard::key(Key::W)) 
    {
        camera.UpdateCameraPos(CameraDirection::FORWARD, m_DeltaTime.count());
    }

    if (Keyboard::key(Key::S)) 
    {
        camera.UpdateCameraPos(CameraDirection::BACKWARD, m_DeltaTime.count());
    }

    if (Keyboard::key(Key::A)) 
    {
        camera.UpdateCameraPos(CameraDirection::LEFT, m_DeltaTime.count());
    }

    if (Keyboard::key(Key::D)) 
    {
        camera.UpdateCameraPos(CameraDirection::RIGHT, m_DeltaTime.count());
    }

    if (Keyboard::key(Key::Space)) 
    {
        camera.UpdateCameraPos(CameraDirection::UP, m_DeltaTime.count());
    }

    if (Keyboard::key(Key::LeftShift)) 
    {
        camera.UpdateCameraPos(CameraDirection::DOWN, m_DeltaTime.count());
    }

    double dx = Mouse::getDX();
    double dy = Mouse::getDY();
    if (dx != 0.0 || dy != 0.0)
        camera.UpdateCameraDirection(dx, dy);

    double scroll = Mouse::getScrollDY();
    if (scroll != 0.0)
        camera.speed += scroll * 3;

    if (Mouse::buttonUp(MouseKey::ButtonLeft))
    {
        if (m_AimedRaycastHit)
        {
            m_VoxelEdit.RemoveVoxel(m_VoxelAimedAt);
        }
    }

    m_MainJoystick.Update();
}

template<typename ChunkT>
void Application<ChunkT>::Update()
{
    PROFILE_FUNCTION();

    Camera& mainCamera = m_CameraManager.GetCamera(m_MainCameraID);

    for (auto& camera : m_CameraManager)
    {
        camera->Update();
    }
    m_Scene.Update(mainCamera.pos, 0);

    m_AimedRaycastHit = VoxelRayCast<ChunkT>::cast(Ray(mainCamera.pos, mainCamera.front, 10), m_World, m_VoxelAimedAt);
}

template<typename ChunkT>
void Application<ChunkT>::Render()
{
    PROFILE_FUNCTION();

    const Camera& mainCamera = m_CameraManager.GetCamera(m_MainCameraID);
    const Camera& activeCamera = m_CameraManager.GetActiveCamera();

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DebugUI::BeginFrame();
    Gizmos::Begin(activeCamera);
    if(m_AimedRaycastHit) Gizmos::DrawCube(glm::vec3(m_VoxelAimedAt) + 0.5f, glm::vec3(1));
    m_DebuggingRenderer.RenderOverlay(mainCamera);
    m_Scene.Render(activeCamera, mainCamera);

    DebugUI::Print("FPS: {}", std::lround(m_FPS));
    DebugUI::Print("Testing 2");
    DebugUI::Print("Testing 3");

    Gizmos::End();
    DebugUI::EndFrame();
}

template<typename ChunkT>
void Application<ChunkT>::Shutdown()
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        std::cerr << "GL ERROR: " << std::hex << err << std::endl;

    DebugUI::Shutdown();
    Gizmos::Shutdown();
    glfwTerminate();
}

#endif