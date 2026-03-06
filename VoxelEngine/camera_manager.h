#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <vector>
#include <memory>

#include "logger.h"

class Camera;

class CameraManager
{
public:
	using CameraId = uint32_t;

	CameraManager();

	CameraId RegisterCamera(std::unique_ptr<Camera> cam);
	void SetActiveCamera(CameraId camID);
	const Camera& GetActiveCamera();
	void Update();

private:
	std::vector<std::unique_ptr<Camera>> m_Cameras;
	CameraId m_ActiveCameraID;
};

#endif

