module;

// TEMP to be moved to thrid_party module & logging, profiling modules
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "logger.h"
#include "profiler.h"

module voxel_engine:input.keyboard.impl;

import :input.keyboard;
import :input.key_codes;

// key state array (true for down, false for up)
bool Keyboard::m_Keys[GLFW_KEY_LAST] = { 0 };
// key changed array (true if changed)
bool Keyboard::m_KeysChanged[GLFW_KEY_LAST] = { 0 };

/*
    static callback
*/

// key state changed
void Keyboard::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    PROFILE_FUNCTION();

    if (action != GLFW_RELEASE)
    {
        if (!m_Keys[key])
        {
            m_Keys[key] = true;

            LOG_DEBUG(EngineSystem::INPUTS,
                "Key pressed: key={} scancode={} mods={}",
                key,
                scancode,
                mods
            );
        }
    }
    else
    {
        m_Keys[key] = false;

        LOG_DEBUG(EngineSystem::INPUTS,
            "Key released: key={} scancode={} mods={}",
            key,
            scancode,
            mods
        );
    }

    m_KeysChanged[key] = action != GLFW_REPEAT;
}

/*
    static accessors
*/

// get key state
bool Keyboard::key(KeyCode key)
{
    LOG_TRACE(EngineSystem::INPUTS,
        "Keyboard::key queried: key={} down={}",
        key,
        m_Keys[key]
    );

    return m_Keys[key];
}

// get if key recently changed
bool Keyboard::keyChanged(KeyCode key)
{
    bool ret = m_KeysChanged[key];
    m_KeysChanged[key] = false;

    LOG_TRACE(EngineSystem::INPUTS,
        "Keyboard::keyChanged queried: key={} changed={}",
        key,
        ret
    );

    return ret;
}

// get if key recently changed and is down
bool Keyboard::keyDown(KeyCode key)
{
    bool result = m_Keys[key] && keyChanged(key);

    LOG_TRACE(EngineSystem::INPUTS,
        "Keyboard::keyDown queried: key={} result={}",
        key,
        result
    );

    return result;
}

// get if key recently changed and is up
bool Keyboard::keyUp(KeyCode key)
{
    bool result = !m_Keys[key] && keyChanged(key);

    LOG_TRACE(EngineSystem::INPUTS,
        "Keyboard::keyUp queried: key={} result={}",
        key,
        result
    );

    return result;
}
