#include "joystick.h"

// TODO: add logging

// generate an instance for joystick with id i
Joystick::Joystick(int i) {
    m_Id = getId(i);

    Update();
}

// update the joystick's states
void Joystick::Update() {
    m_Present = glfwJoystickPresent(m_Id);

    if (m_Present) {
        m_Name = glfwGetJoystickName(m_Id);
        m_Axes = glfwGetJoystickAxes(m_Id, &m_AxesCount);
        m_Buttons = glfwGetJoystickButtons(m_Id, &m_ButtonCount);
    }
}

// get axis value
float Joystick::axesState(JoyStickCode axis) {
    if (m_Present) {
        return m_Axes[axis];
    }

    return -1;
}

// get button state
unsigned char Joystick::buttonState(JoyStickCode button) {
    if (m_Present) {
        return m_Buttons[button];
    }

    return GLFW_RELEASE;
}

// get number of axes
int Joystick::getAxesCount() {
    return m_AxesCount;
}

// get number of buttons
int Joystick::getButtonCount() {
    return m_ButtonCount;
}

// return if joystick present
bool Joystick::isPresent() {
    return m_Present;
}

// get name of joystick
const char* Joystick::getName() {
    return m_Name;
}

// static method to get enum value for joystick
int Joystick::getId(int i) {
    return GLFW_JOYSTICK_1 + i;
}