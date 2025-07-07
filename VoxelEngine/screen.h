#ifndef SCREEN_H
#define SCREEN_H

#include <glad\glad.h>
#include <GLFW\glfw3.h>

#include "shader.h"
#include "mouse.h"
#include "keyboard.h"
#include "joystick.h"

class Screen {
public:
	Screen(unsigned int _width, unsigned int _height, const char* _title) noexcept;
	~Screen();

	bool init();

	void enableInputs();
	void toggleCursor();

	void update();

	void close();
	bool isOpen();
	void setTitle(const char* _title);


	GLFWwindow* getWindow() const { return window; }

	unsigned int getWidth() { return width; }
	unsigned int getHeight() { return height; }

private:
	GLFWwindow* window;
	static unsigned int width;
	static unsigned int height;
	const char* title;

	bool cursorEnabled;

	static void framebuffer_size_callback(GLFWwindow* window, int _width, int _height);
};

#endif // !SCREEN_H
