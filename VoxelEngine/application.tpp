#ifndef VE_APPLICATION_TPP
#define VE_APPLICATION_TPP

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

// Cache configuration
#define CACHE_PAGE_SIZE 50
#define CACHE_NUM_OF_PAGES 125000
#define AVERAGE_NUMBER_OF_INDIRECTCMDS_PER_CHUNK 3

#define LOADED_CHUNK_DISTANCE 9


// TODO make loadedChunkDistance & terrian generation strargy user defiend
template<typename ChunkT>
Application<ChunkT>::Application(unsigned int width, unsigned int height, const char* title)
    : m_MainJoystick(0)
    , m_Camera(glm::vec3(1.0f), width, height, 0.1f, 1000.0f)
    , m_NonUpdateCamera(m_Camera)
    , m_Screen(width, height, title)
    , m_World(m_Camera.pos, LOADED_CHUNK_DISTANCE, std::make_unique<Simple3DPerlinNoiseGeneration<ChunkT>>())
    , m_Renderer(std::make_unique<MesherT>())
    , m_VoxelEdit(m_World, m_Renderer)
    , m_Scene(m_Camera, m_World, m_Renderer)
{}

template<typename ChunkT>
Application<ChunkT>::~Application()
{
    Shutdown();
}

template<typename ChunkT>
bool Application<ChunkT>::Init()
{
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
        CACHE_NUM_OF_PAGES,
        CACHE_PAGE_SIZE,
        static_cast<size_t>(AVERAGE_NUMBER_OF_INDIRECTCMDS_PER_CHUNK * pow(LOADED_CHUNK_DISTANCE, 3))
    );

    m_World.InitializeStartingChunks();
    m_Scene.Init(8);

    return true;
}

template<typename ChunkT>
void Application<ChunkT>::Run()
{
    Profiler::GetInstance().SetEnabled(false);

    while (m_Screen.isOpen())
    {
        PROFILE_SCOPE("Frame");

        double currentTime = glfwGetTime();
        m_DeltaTime = currentTime - m_LastFrameTime;
        m_LastFrameTime = currentTime;
        m_FPS = static_cast<float>(1.0 / m_DeltaTime);

        ProcessInput();
        Update();
        Render();

        m_Screen.Flush();
        //CalcFPSOnWindowTitle(1000);
    }

    Profiler::GetInstance().SetEnabled(true);
}

template<typename ChunkT>
void Application<ChunkT>::ProcessInput()
{
    PROFILE_FUNCTION();

    if (Keyboard::key(Key::Escape))
        m_Screen.close();

    if (Keyboard::keyDown(Key::F1))
    {
        auto& profiler = Profiler::GetInstance();
        profiler.SetEnabled(!profiler.IsEnabled());
    }

    if (Keyboard::keyUp(Key::F2))
    {
        m_FreezeWorld = !m_FreezeWorld;
        m_Scene.SetWorldUpdate(!m_FreezeWorld);

        if (m_FreezeWorld)
        {
            m_NonUpdateCamera.pos = m_Camera.pos;
            m_Scene.SwitchCamera(m_NonUpdateCamera);
        }
        else
        {
            m_Scene.SwitchCamera(m_Camera);
        }
    }

    if (Keyboard::key(Key::W)) m_Camera.UpdateCameraPos(CameraDirection::FORWARD, m_DeltaTime);
    if (Keyboard::key(Key::S)) m_Camera.UpdateCameraPos(CameraDirection::BACKWARD, m_DeltaTime);
    if (Keyboard::key(Key::A)) m_Camera.UpdateCameraPos(CameraDirection::LEFT, m_DeltaTime);
    if (Keyboard::key(Key::D)) m_Camera.UpdateCameraPos(CameraDirection::RIGHT, m_DeltaTime);
    if (Keyboard::key(Key::Space)) m_Camera.UpdateCameraPos(CameraDirection::UP, m_DeltaTime);
    if (Keyboard::key(Key::LeftShift)) m_Camera.UpdateCameraPos(CameraDirection::DOWN, m_DeltaTime);

    double dx = Mouse::getDX();
    double dy = Mouse::getDY();
    if (dx != 0.0 || dy != 0.0)
        m_Camera.UpdateCameraDirection(dx, dy);

    double scroll = Mouse::getScrollDY();
    if (scroll != 0.0)
        m_Camera.UpdateCameraZoom(scroll);

    if (Mouse::buttonUp(MouseKey::ButtonLeft))
    {
        glm::ivec3 voxelCoords;
        bool didHit = VoxelRayCast<ChunkT>::cast(Ray(m_Camera.pos, m_Camera.front, 10), m_World, voxelCoords);
        if (didHit)
        {
            m_VoxelEdit.RemoveVoxel(voxelCoords);
        }
    }

    m_MainJoystick.Update();

}

template<typename ChunkT>
void Application<ChunkT>::Update()
{
    m_Scene.Update();
}

template<typename ChunkT>
void Application<ChunkT>::Render()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_Scene.Render();
}

template<typename ChunkT>
void Application<ChunkT>::Shutdown()
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        std::cerr << "GL ERROR: " << std::hex << err << std::endl;

    glfwTerminate();
}

template<typename ChunkT>
void Application<ChunkT>::CalcFPSOnWindowTitle(int avgOverNFrames)
{
    PROFILE_FUNCTION();

    if (m_CountFPS > avgOverNFrames)
    {
        // this is taking alot of cpu cycles
        std::string title = "VoxelEngine - FPS: " + std::to_string(m_SumFPS / m_CountFPS) +
            " | Pos(" +
            std::to_string(m_Camera.x) + ", " +
            std::to_string(m_Camera.y) + ", " +
            std::to_string(m_Camera.z) + ")";

        m_Screen.setTitle(title.c_str());
        m_CountFPS = 0;
        m_SumFPS = 0;

    }
    else
    {
        m_SumFPS += m_FPS;
        m_CountFPS++;
    }

}

#endif