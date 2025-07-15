#include "camera.h"


Camera::Camera(glm::vec3 position, int _screenWidth, int _screenHeight, float _zNear, float _zFar)
	: pos(position),
		screenWidth(_screenWidth),
		screenHeight(_screenHeight),
		zNear(_zNear),
		zFar(_zFar),
		worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
		yaw(90.0f),
		pitch(0.0f),
		speed(8.0f),
		zoom(45.0f),
		front(glm::vec3(0.0f, 0.0f, -1.0f)) {

	viewMatrix = std::make_shared<glm::mat4>(1.0f);
	projectionMatrix = std::make_shared<glm::mat4>(1.0f);
	camFrustum = std::make_shared<Frustum>();
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

void Camera::update()
{
	*viewMatrix = glm::lookAt(pos, pos + front, up);
	*projectionMatrix = glm::perspective(glm::radians(zoom), (float)screenWidth / screenHeight, zNear, zFar);

	//calculateFrustum();
}

std::weak_ptr<glm::mat4> Camera::getViewMatrixPtr()
{
	return viewMatrix;
}

std::weak_ptr<glm::mat4> Camera::getProjMatrixPtr()
{
	return projectionMatrix;
}

std::weak_ptr<Frustum> Camera::getFrustumPtr()
{
	return camFrustum;
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

void Camera::calculateFrustum()
{
	glm::vec4 row0 = glm::row(*viewMatrix, 0);
	glm::vec4 row1 = glm::row(*viewMatrix, 1);
	glm::vec4 row2 = glm::row(*viewMatrix, 2);
	glm::vec4 row3 = glm::row(*viewMatrix, 3);

	camFrustum->leftClipPlane = row0 + row3;
	camFrustum->rightClipPlane = row0 - row3;
	camFrustum->bottomClipPlane = row1 + row3;
	camFrustum->topClipPlane = row1 - row3;
	camFrustum->nearClipPlane = row2 + row3;
	camFrustum->farClipPlane = row2 - row3;

	// --- Compute minCorner and maxCorner of frustum AABB in world space ---
	glm::mat4 vp = (*projectionMatrix) * (*viewMatrix);
	glm::mat4 invVP = glm::inverse(vp);

	// Clip space cube corners [-1, 1]
	glm::vec3 frustumCorners[8] = {
		{-1.0f, -1.0f, -1.0f},
		{ 1.0f, -1.0f, -1.0f},
		{-1.0f,  1.0f, -1.0f},
		{ 1.0f,  1.0f, -1.0f},
		{-1.0f, -1.0f,  1.0f},
		{ 1.0f, -1.0f,  1.0f},
		{-1.0f,  1.0f,  1.0f},
		{ 1.0f,  1.0f,  1.0f}
	};

	glm::vec3 minCorner(FLT_MAX);
	glm::vec3 maxCorner(-FLT_MAX);

	for (const glm::vec3& corner : frustumCorners) {
		glm::vec4 worldPos = invVP * glm::vec4(corner, 1.0f);
		worldPos /= worldPos.w;

		minCorner = glm::min(minCorner, glm::vec3(worldPos));
		maxCorner = glm::max(maxCorner, glm::vec3(worldPos));
	}

	camFrustum->minCorner = minCorner;
	camFrustum->maxCorner = maxCorner;
}
