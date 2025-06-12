#include "mouse.h"

double Mouse::x = 0;
double Mouse::y = 0;

double Mouse::lastX = 0;
double Mouse::lastY = 0;

double Mouse::dx = 0;
double Mouse::dy = 0;

double Mouse::scrollDX = 0;
double Mouse::scrollDY = 0;

bool Mouse::firstRead = true;

bool Mouse::buttons[GLFW_MOUSE_BUTTON_LAST] = { 0 };
bool Mouse::changedButtons[GLFW_MOUSE_BUTTON_LAST] = { 0 };



void Mouse::cursorPosCallback(GLFWwindow* window, double _x, double _y) {
	x = _x;
	y = _y;

	if (firstRead) {
		lastX = x;
		lastY = y;
		firstRead = false;
	}

	dx = x - lastX;
	dy = lastY - y;

	lastX = x; 
	lastY = y;
}
void Mouse::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (action != GLFW_RELEASE) {
		if (!buttons[button]) {
			buttons[button] = true;
		}
	}
	else {
		buttons[button] = false;
	}

	changedButtons[button] = action != GLFW_REPEAT;

}
void Mouse::mouseWheelCallback(GLFWwindow* window, double dx, double dy) {
	scrollDX = dx;
	scrollDY = dy;

}

double Mouse::getMouseX() {
	return x;
}
double Mouse::getMouseY() {
	return y;
}

double Mouse::getDX() {
	double _dx = dx;
	dx = 0;
	return _dx;
}
double Mouse::getDY() {
	double _dy = dy;
	dy = 0;
	return _dy;
}

double Mouse::getScrollDX() {
	double _scrollDX = scrollDX;
	scrollDX = 0;
	return _scrollDX;
}
double Mouse::getScrollDY() {
	double _scrollDY = scrollDY;
	scrollDY = 0;
	return _scrollDY;
}

bool Mouse::button(int button) {
	return buttons[button];
}
bool Mouse::buttonChanged(int button) {
	bool ret = changedButtons[button];
	changedButtons[button] = false;
	return ret;
}
bool Mouse::buttonUp(int button) {
	return !buttons[button] && buttonChanged(button);
}
bool Mouse::buttonDown(int button) {
	return buttons[button] && buttonChanged(button);
}