#include "mouse.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "logger.h"
#include "profiler.h"

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



void Mouse::cursorPosCallback(GLFWwindow* window, double _x, double _y) 
{
	PROFILE_FUNCTION();

	m_X = _x;
	m_Y = _y;

	if (m_FirstRead) 
	{
		m_LastX = m_X;
		m_LastY = m_Y;
		m_FirstRead = false;

		LOG_INFO(EngineSystem::INPUTS,
			"Mouse first read at position: ({:.2f}, {:.2f})",
			m_X, m_Y
		);
	}

	m_Dx = m_X - m_LastX;
	m_Dy = m_LastY - m_Y;

	LOG_TRACE(EngineSystem::INPUTS,
		"Mouse cursor moved. Delta: ({:.2f}, {:.2f}), Current: ({:.2f}, {:.2f})",
		m_Dx, m_Dy, m_X, m_Y
	);

	m_LastX = m_X; 
	m_LastY = m_Y;
}
void Mouse::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) 
{
	PROFILE_FUNCTION();

	if (action != GLFW_RELEASE) 
	{
		if (!m_Buttons[button]) 
		{
			m_Buttons[button] = true;

			LOG_DEBUG(EngineSystem::INPUTS,
				"Mouse button %d pressed (mods: %d)",
				button, mods
			);
		}
	}
	else 
	{
		m_Buttons[button] = false;
		LOG_DEBUG(EngineSystem::INPUTS,
			"Mouse button %d released (mods: %d)",
			button, mods
		);
	}

	m_ChangedButtons[button] = action != GLFW_REPEAT;

}
void Mouse::mouseWheelCallback(GLFWwindow* window, double m_Dx, double m_Dy) 
{
	PROFILE_FUNCTION();

	m_ScrollDX = m_Dx;
	m_ScrollDY = m_Dy;

	LOG_TRACE(EngineSystem::INPUTS,
		"Mouse wheel scrolled. Delta: ({:.2f}, {:.2f})",
		m_ScrollDX, m_ScrollDY
	);

}

double Mouse::getMouseX() 
{
	LOG_TRACE(EngineSystem::INPUTS,
		"Getting mouse X position: {:.2f}",
		m_X
	);

	return m_X;
}
double Mouse::getMouseY() 
{
	LOG_TRACE(EngineSystem::INPUTS,
		"Getting mouse Y position: {:.2f}",
		m_Y
	);

	return m_Y;
}

double Mouse::getDX() 
{
	double _dx = m_Dx;
	m_Dx = 0;
	return _dx;
}
double Mouse::getDY() 
{
	double _dy = m_Dy;
	m_Dy = 0;
	return _dy;
}

double Mouse::getScrollDX() 
{
    double _scrollDX = m_ScrollDX;
    m_ScrollDX = 0;

    LOG_TRACE(EngineSystem::INPUTS,
        "Getting scroll DX: {:.2f} and resetting it",
        _scrollDX
    );

    return _scrollDX;
}

double Mouse::getScrollDY() 
{
    double _scrollDY = m_ScrollDY;
    m_ScrollDY = 0;

    LOG_TRACE(EngineSystem::INPUTS,
        "Getting scroll DY: {:.2f} and resetting it",
        _scrollDY
    );

    return _scrollDY;
}

bool Mouse::button(MouseCode button) 
{
	LOG_TRACE(EngineSystem::INPUTS,
		"Checking if button %d is pressed: {}",
		button, m_Buttons[button] ? "true" : "false"
	);

	return m_Buttons[button];
}
bool Mouse::buttonChanged(MouseCode button) 
{
	bool ret = m_ChangedButtons[button];
	m_ChangedButtons[button] = false;

	LOG_TRACE(EngineSystem::INPUTS,
		"Checking if button %d state changed: {}",
		button, ret ? "true" : "false"
	);

	return ret;
}

bool Mouse::buttonUp(MouseCode button) 
{
	bool result = !m_Buttons[button] && buttonChanged(button);
	LOG_TRACE(EngineSystem::INPUTS,
		"Checking if button {} was released: {}",
		button, result ? "true" : "false"
	);
	return result;
}

bool Mouse::buttonDown(MouseCode button) 
{
	bool result = m_Buttons[button] && buttonChanged(button);
	LOG_TRACE(EngineSystem::INPUTS,
		"Checking if button %d was pressed: {}",
		button, result ? "true" : "false"
	);
	return result;
}