#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <memory>

enum CameraDirection {
	NONE = 0,
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

struct Frustum
{
	glm::vec4 leftClipPlane;
	glm::vec4 rightClipPlane;
	glm::vec4 bottomClipPlane;
	glm::vec4 topClipPlane;
	glm::vec4 nearClipPlane;
	glm::vec4 farClipPlane;

	bool isPointInFrustum(const glm::vec3& coords)  const
	{
		glm::vec4 paddedCoords(coords, 1.0f);

		bool isInside =
			glm::dot(leftClipPlane, paddedCoords) >= 0 &&
			glm::dot(rightClipPlane, paddedCoords) <= 0 &&
			glm::dot(topClipPlane, paddedCoords) <= 0 &&
			glm::dot(bottomClipPlane, paddedCoords) >= 0 &&
			glm::dot(nearClipPlane, paddedCoords) >= 0 &&
			glm::dot(farClipPlane, paddedCoords) <= 0;

		return isInside;
	}

	bool isAABBInFrustum(const glm::vec3& minPoint, const glm::vec3& maxPoint) const
	{
		glm::vec3 corners[8] = {
		{minPoint.x, minPoint.y, minPoint.z},
		{maxPoint.x, minPoint.y, minPoint.z},
		{minPoint.x, maxPoint.y, minPoint.z},
		{maxPoint.x, maxPoint.y, minPoint.z},
		{minPoint.x, minPoint.y, maxPoint.z},
		{maxPoint.x, minPoint.y, maxPoint.z},
		{minPoint.x, maxPoint.y, maxPoint.z},
		{maxPoint.x, maxPoint.y, maxPoint.z}
		};

		auto isOutsidePlane = [&](const glm::vec4& plane) -> bool {
			for (const glm::vec3& corner : corners) {
				if (glm::dot(plane, glm::vec4(corner, 1.0f)) >= 0.0f) {
					return false;
				}
			}
			return true;
			};

		// If the box is outside any one plane, it's outside the frustum
		if (isOutsidePlane(leftClipPlane)) return false;
		if (isOutsidePlane(rightClipPlane)) return false;
		if (isOutsidePlane(bottomClipPlane)) return false;
		if (isOutsidePlane(topClipPlane)) return false;
		if (isOutsidePlane(nearClipPlane)) return false;
		if (isOutsidePlane(farClipPlane)) return false;

		return true;
	}

};

class Camera {
public:
	glm::vec3 pos;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;

	glm::vec3 worldUp;

	float yaw;
	float pitch;
	float speed;
	float zoom;

	Camera(glm::vec3 position, int _screenWidth, int _screenHeight, float _zNear, float _zFar);
	~Camera() = default;
	
	void UpdateCameraDirection(double dx, double dy);
	void UpdateCameraPos(CameraDirection dir, double dt);
	void UpdateCameraZoom(double dy);
	void Update();

	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetProjMatrix() const;
	Frustum GetFrustum();

private:
	float m_ZNear, m_ZFar;
	int m_ScreenWidth, m_ScreenHeight;

	Frustum m_CamFrustum;
	glm::mat4 m_ViewMatrix;
	glm::mat4 m_ProjectionMatrix;


	void _UpdateCameraVectors();

	void _UpdateFrustum();

};



#endif