#include "screen.h"

unsigned int Screen::width = 0;
unsigned int Screen::height = 0;

Screen::Screen(unsigned int _width, unsigned int _height, const char* _title) noexcept {
	window = nullptr;
	Screen::width = _width;
	Screen::height = _height;
	title = _title;
	cursorEnabled = true;
}

Screen::~Screen() {
	if (window) {
		glfwDestroyWindow(window);
		glfwTerminate();
	}
}

bool Screen::init() {
	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW\n";
		return false;
	}

	// set version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	window = glfwCreateWindow(Screen::width, Screen::height, title, NULL, NULL);

	if (!window) {
		std::cout << "failed to create window\n";
		return false;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "failed to init glad\n";
		return false;
	}

	glViewport(0, 0, width, height);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


	return true;
}

void Screen::enableInputs() {
	glfwSetKeyCallback(window, Keyboard::keyCallback);
	glfwSetCursorPosCallback(window, Mouse::cursorPosCallback);
	glfwSetMouseButtonCallback(window, Mouse::mouseButtonCallback);
	glfwSetScrollCallback(window, Mouse::mouseWheelCallback);
}

void Screen::toggleCursor() {
	cursorEnabled = !cursorEnabled;
	if (cursorEnabled) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
	else {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
}

void Screen::update() {
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void Screen::close() {
	glfwSetWindowShouldClose(window, true);
}
bool Screen::isOpen() {
	return !glfwWindowShouldClose(window);
}

void Screen::setTitle(const char* newTitle)
{
	title = newTitle;

	if (window) {
		glfwSetWindowTitle(window, newTitle);
	}
}

void Screen::framebuffer_size_callback(GLFWwindow* window, int _width, int _height) {
	glViewport(0, 0, _width, _height);
	Screen::width = _width;
	Screen::height = _height;
}