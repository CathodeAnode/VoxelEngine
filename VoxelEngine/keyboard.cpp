#include "keyboard.h"

// key state array (true for down, false for up)
bool Keyboard::m_Keys[GLFW_KEY_LAST] = { 0 };
// key changed array (true if changed)
bool Keyboard::m_KeysChanged[GLFW_KEY_LAST] = { 0 };

/*
    static callback
*/

// key state changed
void Keyboard::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_RELEASE) {
        if (!m_Keys[key]) {
            m_Keys[key] = true;
        }
    }
    else {
        m_Keys[key] = false;
    }
    m_KeysChanged[key] = action != GLFW_REPEAT;
}

/*
    static accessors
*/

// get key state
bool Keyboard::key(int key) {
    return m_Keys[key];
}

// get if key recently changed
bool Keyboard::keyChanged(int key) {
    bool ret = m_KeysChanged[key];
    // set to false because change no longer new
    m_KeysChanged[key] = false;
    return ret;
}

// get if key recently changed and is up
bool Keyboard::keyDown(int key) {
    return m_Keys[key] && keyChanged(key);
}

// get if key recently changed and is down
bool Keyboard::keyUp(int key) {
    return !m_Keys[key] && keyChanged(key);
}