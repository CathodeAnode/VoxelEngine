#ifndef JOYSTICK_H
#define JOYSTICK_H


#include <GLFW/glfw3.h>

#include "joystick_codes.h"

/*
    joystick class to handle input from joystick controller
*/

class Joystick {
public:
    // generate an instance for joystick with id i
    Joystick(int i);

    // update the joystick's states
    void Update();


    float axesState(JoyStickCode axis); // get axis value
    unsigned char buttonState(JoyStickCode button); // get button state

    int getAxesCount(); // get number of axes
    int getButtonCount(); // get number of buttons

    bool isPresent(); // return if joystick present
    const char* getName(); // get name of joystick

    static int getId(int i); // static method to get enum value for joystick

private:
    int m_Present; // 1 if present, 0 if not
 
    int m_Id; // joystick id
    const char* m_Name; // joystick name

    int m_AxesCount; // number of axes on joystick
    const float* m_Axes; // array of axes values

    int m_ButtonCount; // number of buttons
    const unsigned char* m_Buttons; // array of button states
};


#endif

