module;

#include <cstdint>

export module voxel_engine:input.joystick_codes;

export using JoyStickCode = std::uint16_t;

export namespace JoystickKey
{
	enum : JoyStickCode
	{
		// analog input	button values		PS		|		XBOX
		BTN_LEFT = 0,				//	Square		|	X
		BTN_DOWN = 1,				//	X			|	A
		BTN_RIGHT = 2,				//	Circle		|	B
		BTN_UP = 3,					//	Triangle	|	Y	
		SHOULDER_LEFT = 4,			//	L1			|	LB
		SHOULDER_RIGHT = 5,			//	R1			|	RB
		TRIGGER_LEFT = 6,			//	L2			|	LT
		TRIGGER_RIGHT = 7,			//	R2			|	RT
		SELECT = 8,					//	Share		|	Address
		START = 9,					//	Options		|	Menu
		LEFT_STICK = 10,			//	L3			|	LS
		RIGHT_STICK=  11,			//	R3			|	RS
		HOME = 12,					//	Home		|	Home
		CLICK = 13,					//	Touch pad	|	n/a
		DPAD_UP = 14,				//	Dpad up		|	Dpad up
		DPAD_RIGHT = 15,			//	Dpad right	|	Dpad right
		DPAD_DOWN = 16,				//	Dpad down	|	Dpad down
		DPAD_LEFT = 17,				//	Dpad left	|	Dpad left

		// axes
		LEFT_STICK_X = 0,
		LEFT_STICK_Y = 1,
		RIGHT_STICK_X = 2,
		LEFT_TRIGGER = 3,
		RIGHT_TRIGGER = 4,
		RIGHT_STICK_Y = 5,
	};
}