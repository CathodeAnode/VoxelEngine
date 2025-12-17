#ifndef MOUSE_H
#define MOUSE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define 	MOUSE_BUTTON_1   0
#define 	MOUSE_BUTTON_2   1
#define 	MOUSE_BUTTON_3   2
#define 	MOUSE_BUTTON_4   3
#define 	MOUSE_BUTTON_5   4
#define 	MOUSE_BUTTON_6   5
#define 	MOUSE_BUTTON_7   6
#define 	MOUSE_BUTTON_8   7

#define 	MOUSE_BUTTON_LAST		MOUSE_BUTTON_8
#define 	MOUSE_BUTTON_LEFT		MOUSE_BUTTON_1
#define 	MOUSE_BUTTON_RIGHT		MOUSE_BUTTON_2
#define 	MOUSE_BUTTON_MIDDLE		MOUSE_BUTTON_3

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
