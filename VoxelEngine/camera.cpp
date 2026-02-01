#include "camera.h"
#include <iostream>


Camera::Camera(glm::vec3 position, int _screenWidth, int _screenHeight, float _zNear, float _zFar)
	: pos(position)
	, m_ScreenWidth(_screenWidth)
	, m_ScreenHeight(_screenHeight)
	, m_ZNear(_zNear)
	, m_ZFar(_zFar)
	, worldUp(glm::vec3(0.0f, 1.0f, 0.0f))
	, yaw(90.0f)
	, pitch(0.0f)
	, speed(8.0f)
	, zoom(45.0f)
	, front(glm::vec3(0.0f, 0.0f, -1.0f))
	, m_ViewMatrix(1.0f)
	, m_ProjectionMatrix(1.0f)
	, m_CamFrustum()
{
	_UpdateCameraVectors();
}


void Camera::UpdateCameraDirection(double dx, double dy) {
	yaw += dx;
	pitch += dy;

	if (pitch > 89.0f) {
		pitch = 89.0f;
	}
	else if (pitch < -89.0f) {
		pitch = -89.0f;
	}

	_UpdateCameraVectors();
}
void Camera::UpdateCameraPos(CameraDirection dir, double dt) {
	float velocity = (float)dt * speed;

	switch (dir)
	{
	case NONE:
		break;
	case FORWARD:
		pos += front * velocity;
		break;
	case BACKWARD:
		pos -= front * velocity;
		break;
	case LEFT:
		pos -= right * velocity;
		break;
	case RIGHT:
		pos += right * velocity;
		break;
	case UP:
		pos += worldUp * velocity;
		break;
	case DOWN:
		pos -= worldUp * velocity;
		break;
	default:
		break;
	}
}
void Camera::UpdateCameraZoom(double dy) {
	if (zoom >= 1.0f && zoom <= 45.0f) {
		zoom -= dy;
	}
	else if (zoom < 1.0f) {
		zoom = 1.0f;
	}
	else {
		zoom = 45.0f;
	}
}

void Camera::Update()
{
	PROFILE_FUNCTION();

	m_ViewMatrix = glm::lookAt(pos, pos + front, up);
	m_ProjectionMatrix = glm::perspective(glm::radians(zoom), (float)m_ScreenWidth / m_ScreenHeight, m_ZNear, m_ZFar);

	_UpdateFrustum();
}

glm::mat4 Camera::GetViewMatrix() const
{
	return m_ViewMatrix;
}

glm::mat4 Camera::GetProjMatrix() const
{
	return m_ProjectionMatrix;
}

const Frustum& Camera::GetFrustum() const
{
	return m_CamFrustum;
}

void Camera::_UpdateCameraVectors() {
	glm::vec3 direction;

	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	front = glm::normalize(direction);
	right = glm::normalize(glm::cross(front, worldUp));
	up = glm::normalize(glm::cross(right, front));
}

void Camera::_UpdateFrustum()
{
	glm::vec4 row0 = glm::row(m_ViewMatrix, 0);
	glm::vec4 row1 = glm::row(m_ViewMatrix, 1);
	glm::vec4 row2 = glm::row(m_ViewMatrix, 2);
	glm::vec4 row3 = glm::row(m_ViewMatrix, 3);

	m_CamFrustum.leftClipPlane = row0 + row3;
	m_CamFrustum.rightClipPlane = row0 - row3;
	m_CamFrustum.bottomClipPlane = row1 + row3;
	m_CamFrustum.topClipPlane = row1 - row3;
	m_CamFrustum.nearClipPlane = row2 + row3;
	m_CamFrustum.farClipPlane = row2 - row3;
}
