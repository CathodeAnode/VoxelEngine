#include "joystick.h"
#include "logger.h"

#include <GLFW/glfw3.h>

// generate an instance for joystick with id i
Joystick::Joystick(int i) 
{
    m_Id = getId(i);

    Update();

    LOG_INFO(EngineSystem::INPUTS,
        "Joystick {} initialized with id: {}",
        i, m_Id
    );
}

// update the joystick's states
void Joystick::Update() 
{
    m_Present = glfwJoystickPresent(m_Id);

    if (m_Present) 
    {
        m_Name = glfwGetJoystickName(m_Id);
        m_Axes = glfwGetJoystickAxes(m_Id, &m_AxesCount);
        m_Buttons = glfwGetJoystickButtons(m_Id, &m_ButtonCount);

        LOG_INFO(EngineSystem::INPUTS,
            "Joystick {} detected: {}, {} axes, {} buttons",
            m_Id, m_Name, m_AxesCount, m_ButtonCount
        );
    }
}

// get axis value
float Joystick::axesState(JoyStickCode axis) 
{
    if (m_Present) 
    {
        float axisValue = m_Axes[axis];
        LOG_TRACE(EngineSystem::INPUTS,
            "Joystick {} axis {} state: {:.2f}",
            m_Id, axis, axisValue
        );
        return axisValue;
    }

    LOG_WARN(EngineSystem::INPUTS,
        "Joystick {} is not present. Returning -1 for axis {}.",
        m_Id, axis
    );

    return -1;
}

// get button state
unsigned char Joystick::buttonState(JoyStickCode button) 
{
    if (m_Present) 
    {
        unsigned char buttonState = m_Buttons[button];
        LOG_TRACE(EngineSystem::INPUTS,
            "Joystick {} button {} state: {}",
            m_Id, button, buttonState == GLFW_PRESS ? "pressed" : "released"
        );
        return buttonState;
    }

    LOG_WARN(EngineSystem::INPUTS,
        "Joystick {} is not present. Returning GLFW_RELEASE for button {}.",
        m_Id, button
    );

    return GLFW_RELEASE;
}

// get number of axes
int Joystick::getAxesCount() 
{
    LOG_TRACE(EngineSystem::INPUTS,
        "Joystick {} has {} axes.",
        m_Id, m_AxesCount
    );
    return m_AxesCount;
}

// get number of buttons
int Joystick::getButtonCount() 
{
    LOG_TRACE(EngineSystem::INPUTS,
        "Joystick {} has {} buttons.",
        m_Id, m_ButtonCount
    );
    return m_ButtonCount;
}

// return if joystick present
bool Joystick::isPresent() 
{
    LOG_TRACE(EngineSystem::INPUTS,
        "Joystick {} present: {}",
        m_Id, m_Present ? "yes" : "no"
    );
    return m_Present;
}

// get name of joystick
const char* Joystick::getName() 
{
    LOG_TRACE(EngineSystem::INPUTS,
        "Joystick {} name: {}",
        m_Id, m_Name
    );
    return m_Name;
}

// static method to get enum value for joystick
int Joystick::getId(int i) 
{
    return GLFW_JOYSTICK_1 + i;
}