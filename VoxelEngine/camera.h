#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
	
	void updateCameraDirection(double dx, double dy);
	void updateCameraPos(CameraDirection dir, double dt);
	void updateCameraZoom(double dy);
	void update();

	std::weak_ptr<glm::mat4> getViewMatrixPtr();
	std::weak_ptr<glm::mat4> getProjMatrixPtr();


private:
	float zNear, zFar;
	int screenWidth, screenHeight;

	std::shared_ptr<glm::mat4> viewMatrix;
	std::shared_ptr<glm::mat4> projectionMatrix;

	void updateCameraVectors();

};

#endif