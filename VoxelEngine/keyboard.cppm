module;

// TEMP to be moved to thrid_party module & logging, profiling modules
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "logger.h"
#include "profiler.h"

export module voxel_engine:input.keyboard;

import :input.key_codes;

export class Keyboard 
{
public:
	//key state callback
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	// static accessors

	// get key state
	static bool key(KeyCode key);

	// get if key changed
	static bool keyChanged(KeyCode key);

	// get if key went up
	static bool keyUp(KeyCode key);

	// get if key went down
	static bool keyDown(KeyCode key);

private:
	static bool m_Keys[]; // key state array (true for down, false for up)
	static bool m_KeysChanged[]; // key changed array (true if changed)
};