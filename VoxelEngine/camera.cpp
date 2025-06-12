#include "camera.h"


Camera::Camera(glm::vec3 position)
	: pos(position),
		worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
		yaw(-90.0f),
		pitch(0.0f),
		speed(1.5f),
		zoom(45.0f),
		front(glm::vec3(0.0f, 0.0f, -1.0f)) {
	updateCameraVectors();
}

void Camera::updateCameraDirection(double dx, double dy) {
	yaw += dx;
	pitch += dy;

	if (pitch > 89.0f) {
		pitch = 89.0f;
	}
	else if (pitch < -89.0f) {
		pitch = -89.0f;
	}

	updateCameraVectors();
}
void Camera::updateCameraPos(CameraDirection dir, double dt) {
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
void Camera::updateCameraZoom(double dy) {
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

glm::mat4 Camera::getViewMatrix() {
	return glm::lookAt(pos, pos + front, up);
}

void Camera::updateCameraVectors() {
	glm::vec3 direction;

	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	front = glm::normalize(direction);
	right = glm::normalize(glm::cross(front, worldUp));
	up = glm::normalize(glm::cross(right, front));
}