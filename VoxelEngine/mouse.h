#ifndef MOUSE_H
#define MOUSE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Mouse {
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

	static bool button(int button);
	static bool buttonChanged(int button);
	static bool buttonUp(int button);
	static bool buttonDown(int button);

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



#endif // !MOUSE_H
