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

Camera& CameraManager::GetActiveCamera()
{
    assert(m_ActiveCameraID < m_Cameras.size() && "Invalid active camera ID");
    return *m_Cameras[m_ActiveCameraID];
}

const Camera& CameraManager::GetActiveCamera() const
{
    assert(m_ActiveCameraID < m_Cameras.size() && "Invalid active camera ID");

    return *m_Cameras[m_ActiveCameraID];
}

Camera& CameraManager::GetCamera(CameraId camID)
{
    assert(m_ActiveCameraID < m_Cameras.size() && "Invalid active camera ID");

    return *m_Cameras[camID];
}

const Camera& CameraManager::GetCamera(CameraId camID) const
{
    assert(m_ActiveCameraID < m_Cameras.size() && "Invalid active camera ID");

    return *m_Cameras[camID];
}

void CameraManager::Update()
{
    m_Cameras[m_ActiveCameraID]->Update();
}