module;

// TEMP to be moved to thrid_party module & logging, profiling modules
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "logger.h"
#include "profiler.h"

export module voxel_engine:input.mouse;

import :input.mouse_codes;

export class Mouse 
{
public:
	static void cursorPosCallback(GLFWwindow* window, double _x, double _y);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void mouseWheelCallback(GLFWwindow* window, double m_Dx, double m_Dy);

	static double getMouseX();
	static double getMouseY();

	static double getDX();
	static double getDY();

	static double getScrollDX();
	static double getScrollDY();

	static bool button(MouseCode button);
	static bool buttonChanged(MouseCode button);
	static bool buttonUp(MouseCode button);
	static bool buttonDown(MouseCode button);

private:
	static double m_X;
	static double m_Y;

	static double m_LastX;
	static double m_LastY;

	static double m_Dx;
	static double m_Dy;

	static double m_ScrollDX;
	static double m_ScrollDY;

	static bool m_FirstRead;

	static bool m_Buttons[];
	static bool m_ChangedButtons[];
};

