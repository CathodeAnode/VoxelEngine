#include "camera_manager.h"
#include "camera.h"


CameraManager::CameraManager()
    : m_ActiveCameraID(-1)
{
}

CameraManager::CameraId CameraManager::RegisterCamera(std::unique_ptr<Camera> cam)
{
    if (!cam)
        LOG_ERROR(EngineSystem::CORE, "RegisterCamera received null camera");

    m_Cameras.push_back(std::move(cam));
    CameraId id = m_Cameras.size() - 1;

    // If this is the first camera, make it active
    if (m_ActiveCameraID == -1)
        m_ActiveCameraID = id;

    return id;
}

void CameraManager::SetActiveCamera(CameraId camID)
{
    if (camID >= m_Cameras.size())
        LOG_ERROR(EngineSystem::CORE, "Invalid CameraId");

    m_ActiveCameraID = camID;
}

const Camera& CameraManager::GetActiveCamera()
{
    if (m_ActiveCameraID >= m_Cameras.size())
        LOG_ERROR(EngineSystem::CORE, "No active camera");

    return *m_Cameras[m_ActiveCameraID];
}

void CameraManager::Update()
{
    m_Cameras[m_ActiveCameraID]->Update();
}