#ifndef SCREEN_H
#define SCREEN_H

struct GLFWwindow;

#define SCREEN_OPENGL_MAJOR_VERISON 4
#define SCREEN_OPENGL_MINOR_VERISON 6

using KeyCallbackFn = void(*)(GLFWwindow*, int, int, int, int);
using CursorPosFn = void(*)(GLFWwindow*, double, double);
using MouseButtonFn = void(*)(GLFWwindow*, int, int, int);
using ScrollFn = void(*)(GLFWwindow*, double, double);


class Screen {
public:
	Screen(unsigned int _width, unsigned int _height, const char* _title) noexcept;
	~Screen();

	bool init();

	void enableInputs(KeyCallbackFn key, CursorPosFn cursor, MouseButtonFn button, ScrollFn scroll);
	void toggleCursor();

	void Flush();

	void close();
	bool isOpen();
	void setTitle(const char* _title);


	GLFWwindow* getWindow() const { return m_Window; }

	unsigned int getWidth() { return m_Width; }
	unsigned int getHeight() { return m_Height; }

private:
	GLFWwindow* m_Window;
	static unsigned int m_Width;
	static unsigned int m_Height;
	const char* m_Title;

	bool m_CursorEnabled;

private:
	static void framebuffer_size_callback(GLFWwindow* window, int _width, int _height);
	static void log_Opengl_info();
};

#endif // !SCREEN_H
