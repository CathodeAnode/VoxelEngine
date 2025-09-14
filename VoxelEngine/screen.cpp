#include "screen.h"

unsigned int Screen::m_Width = 0;
unsigned int Screen::m_Height = 0;

Screen::Screen(unsigned int _width, unsigned int _height, const char* _title) noexcept {
	m_Window = nullptr;
	Screen::m_Width = _width;
	Screen::m_Height = _height;
	m_Title = _title;
	m_CursorEnabled = true;
}

Screen::~Screen() {
	if (m_Window) {
		glfwDestroyWindow(m_Window);
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

	m_Window = glfwCreateWindow(Screen::m_Width, Screen::m_Height, m_Title, NULL, NULL);

	if (!m_Window) {
		std::cout << "failed to create m_Window\n";
		return false;
	}

	glfwMakeContextCurrent(m_Window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "failed to init glad\n";
		return false;
	}

	glViewport(0, 0, m_Width, m_Height);
	glfwSetFramebufferSizeCallback(m_Window, framebuffer_size_callback);


	return true;
}

void Screen::enableInputs() {
	glfwSetKeyCallback(m_Window, Keyboard::keyCallback);
	glfwSetCursorPosCallback(m_Window, Mouse::cursorPosCallback);
	glfwSetMouseButtonCallback(m_Window, Mouse::mouseButtonCallback);
	glfwSetScrollCallback(m_Window, Mouse::mouseWheelCallback);
}

void Screen::toggleCursor() {
	m_CursorEnabled = !m_CursorEnabled;
	if (m_CursorEnabled) {
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
	else {
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
}

void Screen::Update() {
	glfwSwapBuffers(m_Window);
	glfwPollEvents();
}

void Screen::close() {
	glfwSetWindowShouldClose(m_Window, true);
}
bool Screen::isOpen() {
	return !glfwWindowShouldClose(m_Window);
}

void Screen::setTitle(const char* newTitle)
{
	m_Title = newTitle;

	if (m_Window) {
		glfwSetWindowTitle(m_Window, newTitle);
	}
}

void Screen::framebuffer_size_callback(GLFWwindow* m_Window, int _width, int _height) {
	glViewport(0, 0, _width, _height);
	Screen::m_Width = _width;
	Screen::m_Height = _height;
}