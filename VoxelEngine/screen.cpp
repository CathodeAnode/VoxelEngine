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
	LOG_INFO(EngineSystem::CORE, "Destroying Screen");

	if (m_Window) {
		glfwDestroyWindow(m_Window);
		glfwTerminate();
		LOG_INFO(EngineSystem::CORE, "GLFW terminated successfully");
	}
}

bool Screen::init() {
	LOG_INFO(EngineSystem::CORE, "Screen created ({}x{}) with title '{}'", m_Width, m_Height, m_Title);

	if (!glfwInit()) {
		LOG_ERROR(EngineSystem::CORE, "Failed to initialize GLFW");
		return false;
	}

	LOG_INFO(EngineSystem::CORE, "GLFW initialized");
	// set version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, SCREEN_OPENGL_MAJOR_VERISON);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, SCREEN_OPENGL_MINOR_VERISON);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	LOG_INFO(EngineSystem::CORE, "Using forward-compatible OpenGL for macOS");
#endif

	m_Window = glfwCreateWindow(Screen::m_Width, Screen::m_Height, m_Title, NULL, NULL);

	if (!m_Window) {
		LOG_ERROR(EngineSystem::CORE, "Failed to create GLFW window");
		return false;
	}

	LOG_INFO(EngineSystem::CORE, "GLFW window created ({}x{})", m_Width, m_Height);

	glfwMakeContextCurrent(m_Window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		LOG_ERROR(EngineSystem::CORE, "Failed to initialize GLAD");
		return false;
	}

	LOG_INFO(EngineSystem::CORE, "GLAD initialized");

	glViewport(0, 0, m_Width, m_Height);
	glfwSetFramebufferSizeCallback(m_Window, framebuffer_size_callback);

	LOG_INFO(EngineSystem::CORE, "Screen initialization complete");
	log_Opengl_info();

	return true;
}

void Screen::enableInputs() {
	LOG_INFO(EngineSystem::CORE, "Enabling input callbacks");
	glfwSetKeyCallback(m_Window, Keyboard::keyCallback);
	glfwSetCursorPosCallback(m_Window, Mouse::cursorPosCallback);
	glfwSetMouseButtonCallback(m_Window, Mouse::mouseButtonCallback);
	glfwSetScrollCallback(m_Window, Mouse::mouseWheelCallback);
}

void Screen::toggleCursor() {
	m_CursorEnabled = !m_CursorEnabled;
	if (m_CursorEnabled) {
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		LOG_INFO(EngineSystem::CORE, "Cursor enabled");
	}
	else {
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		LOG_INFO(EngineSystem::CORE, "Cursor disabled");
	}
}

void Screen::Flush() {
	PROFILE_FUNCTION();

	glfwSwapBuffers(m_Window);
	glfwPollEvents();
}

void Screen::close() {
	LOG_INFO(EngineSystem::CORE, "Closing window requested");
	glfwSetWindowShouldClose(m_Window, true);
}
bool Screen::isOpen() {
	return !glfwWindowShouldClose(m_Window);
}

void Screen::setTitle(const char* newTitle)
{
	LOG_INFO(EngineSystem::CORE, "Changing window title to '{}'", newTitle);

	m_Title = newTitle;

	if (m_Window) {
		glfwSetWindowTitle(m_Window, newTitle);
	}
}

void Screen::framebuffer_size_callback(GLFWwindow* m_Window, int _width, int _height) {
	LOG_INFO(EngineSystem::CORE, "Framebuffer resized to {}x{}", _width, _height);
	glViewport(0, 0, _width, _height);
	Screen::m_Width = _width;
	Screen::m_Height = _height;
}

void Screen::log_Opengl_info()
{
	const GLubyte* vendor = glGetString(GL_VENDOR);
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	const GLubyte* glsl = glGetString(GL_SHADING_LANGUAGE_VERSION);

	LOG_INFO(EngineSystem::CORE, "OpenGL Vendor   : {}", reinterpret_cast<const char*>(vendor));
	LOG_INFO(EngineSystem::CORE, "OpenGL Renderer : {}", reinterpret_cast<const char*>(renderer));
	LOG_INFO(EngineSystem::CORE, "OpenGL Version  : {}", reinterpret_cast<const char*>(version));
	LOG_INFO(EngineSystem::CORE, "GLSL Version    : {}", reinterpret_cast<const char*>(glsl));
}
