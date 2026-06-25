#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <vector>
#include <memory>

class Camera;

class CameraManager
{
public:
	using CameraId = int32_t;

	CameraManager();

	CameraId RegisterCamera(std::unique_ptr<Camera> cam);
	void SetActiveCamera(CameraId camID);
	Camera& GetActiveCamera();
	const Camera& GetActiveCamera() const;

	Camera& GetCamera(CameraId camID);
	const Camera& GetCamera(CameraId camID) const;

	void Update();

	// expose container iterators
	auto begin() { return m_Cameras.begin(); }
	auto end() { return m_Cameras.end(); }

	auto begin() const { return m_Cameras.begin(); }
	auto end() const { return m_Cameras.end(); }

private:
	std::vector<std::unique_ptr<Camera>> m_Cameras;
	CameraId m_ActiveCameraID;
};

#endif

