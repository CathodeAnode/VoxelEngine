#ifndef JOYSTICK_H
#define JOYSTICK_H


#include <GLFW/glfw3.h>

// analog input	button values					//		PS		|		XBOX
#define GLFW_JOYSTICK_BTN_LEFT 0				//	Square		|	X
#define GLFW_JOYSTICK_BTN_DOWN 1				//	X			|	A
#define GLFW_JOYSTICK_BTN_RIGHT 2				//	Circle		|	B
#define GLFW_JOYSTICK_BTN_UP 3					//	Triangle	|	Y	
#define GLFW_JOYSTICK_SHOULDER_LEFT 4			//	L1			|	LB
#define GLFW_JOYSTICK_SHOULDER_RIGHT 5			//	R1			|	RB
#define GLFW_JOYSTICK_TRIGGER_LEFT 6			//	L2			|	LT
#define GLFW_JOYSTICK_TRIGGER_RIGHT 7			//	R2			|	RT
#define GLFW_JOYSTICK_SELECT 8					//	Share		|	Address
#define GLFW_JOYSTICK_START 9					//	Options		|	Menu
#define GLFW_JOYSTICK_LEFT_STICK 10				//	L3			|	LS
#define GLFW_JOYSTICK_RIGHT_STICK 11			//	R3			|	RS
#define GLFW_JOYSTICK_HOME 12					//	Home		|	Home
#define GLFW_JOYSTICK_CLICK 13					//	Touch pad	|	n/a
#define GLFW_JOYSTICK_DPAD_UP 14				//	Dpad up		|	Dpad up
#define GLFW_JOYSTICK_DPAD_RIGHT 15				//	Dpad right	|	Dpad right
#define GLFW_JOYSTICK_DPAD_DOWN 16				//	Dpad down	|	Dpad down
#define GLFW_JOYSTICK_DPAD_LEFT 17				//	Dpad left	|	Dpad left

// axes
#define GLFW_JOYSTICK_AXES_LEFT_STICK_X 0
#define GLFW_JOYSTICK_AXES_LEFT_STICK_Y 1
#define GLFW_JOYSTICK_AXES_RIGHT_STICK_X 2
#define GLFW_JOYSTICK_AXES_LEFT_TRIGGER 3
#define GLFW_JOYSTICK_AXES_RIGHT_TRIGGER 4
#define GLFW_JOYSTICK_AXES_RIGHT_STICK_Y 5

/*
    joystick class to handle input from joystick controller
*/

class Joystick {
public:
    // generate an instance for joystick with id i
    Joystick(int i);

    // update the joystick's states
    void update();


    float axesState(int axis); // get axis value
    unsigned char buttonState(int button); // get button state

    int getAxesCount(); // get number of axes
    int getButtonCount(); // get number of buttons

    bool isPresent(); // return if joystick present
    const char* getName(); // get name of joystick

    static int getId(int i); // static method to get enum value for joystick

private:
    int present; // 1 if present, 0 if not
 
    int id; // joystick id
    const char* name; // joystick name

    int axesCount; // number of axes on joystick
    const float* axes; // array of axes values

    int buttonCount; // number of buttons
    const unsigned char* buttons; // array of button states
};


#endif

