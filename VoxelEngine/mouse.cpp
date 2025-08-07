#include "mouse.h"

double Mouse::m_X = 0;
double Mouse::m_Y = 0;

double Mouse::m_LastX = 0;
double Mouse::m_LastY = 0;

double Mouse::m_Dx = 0;
double Mouse::m_Dy = 0;

double Mouse::m_ScrollDX = 0;
double Mouse::m_ScrollDY = 0;

bool Mouse::m_FirstRead = true;

bool Mouse::m_Buttons[GLFW_MOUSE_BUTTON_LAST] = { 0 };
bool Mouse::m_ChangedButtons[GLFW_MOUSE_BUTTON_LAST] = { 0 };



void Mouse::cursorPosCallback(GLFWwindow* window, double _x, double _y) {
	m_X = _x;
	m_Y = _y;

	if (m_FirstRead) {
		m_LastX = m_X;
		m_LastY = m_Y;
		m_FirstRead = false;
	}

	m_Dx = m_X - m_LastX;
	m_Dy = m_LastY - m_Y;

	m_LastX = m_X; 
	m_LastY = m_Y;
}
void Mouse::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (action != GLFW_RELEASE) {
		if (!m_Buttons[button]) {
			m_Buttons[button] = true;
		}
	}
	else {
		m_Buttons[button] = false;
	}

	m_ChangedButtons[button] = action != GLFW_REPEAT;

}
void Mouse::mouseWheelCallback(GLFWwindow* window, double m_Dx, double m_Dy) {
	m_ScrollDX = m_Dx;
	m_ScrollDY = m_Dy;

}

double Mouse::getMouseX() {
	return m_X;
}
double Mouse::getMouseY() {
	return m_Y;
}

double Mouse::getDX() {
	double _dx = m_Dx;
	m_Dx = 0;
	return _dx;
}
double Mouse::getDY() {
	double _dy = m_Dy;
	m_Dy = 0;
	return _dy;
}

double Mouse::getScrollDX() {
	double _scrollDX = m_ScrollDX;
	m_ScrollDX = 0;
	return _scrollDX;
}
double Mouse::getScrollDY() {
	double _scrollDY = m_ScrollDY;
	m_ScrollDY = 0;
	return _scrollDY;
}

bool Mouse::button(int button) {
	return m_Buttons[button];
}
bool Mouse::buttonChanged(int button) {
	bool ret = m_ChangedButtons[button];
	m_ChangedButtons[button] = false;
	return ret;
}
bool Mouse::buttonUp(int button) {
	return !m_Buttons[button] && buttonChanged(button);
}
bool Mouse::buttonDown(int button) {
	return m_Buttons[button] && buttonChanged(button);
}