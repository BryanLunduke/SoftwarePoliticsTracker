#ifndef MOUSE_H__
#define MOUSE_H__

#include <vector>
#include "../log.h"


/** A mouse action such as a button press, release or mouse movement */
class MouseAction
{
public:
	static const char ACTION_MOVE = 0;
	static const char ACTION_LEFT = 1;
	static const char ACTION_RIGHT = 2;
	static const char ACTION_WHEEL = 3;

	static const char DATA_UP = 0;
	static const char DATA_DOWN = 1;

	MouseAction(char actionButtonId, char buttonData, short x, short y, char pointerId);
	MouseAction(char actionButtonId, char buttonData, short x, short y, short dx, short dy, char pointerId);

	bool isButton() const;

	short x, y;
	short dx, dy;
	char action;
	char data;
	char pointerId;
};

/* Iterators */
typedef std::vector<MouseAction> MouseActionVec;
typedef MouseActionVec::iterator MouseActionIt;
typedef MouseActionVec::const_iterator MouseActionCIt;


class MouseDevice
{
public:
	static const int MAX_NUM_BUTTONS = 4;

	MouseDevice();
	
	char getButtonState(int buttonId);
	bool isButtonDown(int buttonId);

	/// Was the current movement the first movement after mouse down?
	bool wasFirstMovement();

	short getX();
	short getY();

	short getDX();
	short getDY();

	void reset();
	void reset2();

	bool next();
	void rewind();

	bool getEventButtonState();
	char getEventButton();
	const MouseAction& getEvent();

	void feed(char actionButtonId, char buttonData, short x, short y);
	void feed(char actionButtonId, char buttonData, short x, short y, short dx, short dy);

private:
	int _index;
	short _x, _y;
	short _dx, _dy;
	short _xOld, _yOld;
	char _buttonStates[MAX_NUM_BUTTONS];
	std::vector<MouseAction> _inputs;
	int _firstMovementType;
    	static const int DELTA_NOTSET = -9999;
};

/** A static mouse class, written to resemble the one in lwjgl somewhat
    UPDATE: which is very silly, since it doesn't support multi touch... */
class Mouse
{
public:
	static char getButtonState(int buttonId);
	static bool isButtonDown(int buttonId);

	static short getX();
	static short getY();

	static short getDX();
	static short getDY();

	static void reset();
	static void reset2();

	static bool next();
	static void rewind();

	static bool getEventButtonState();
	static char getEventButton();
	static const MouseAction& getEvent();

	static void feed(char actionButtonId, char buttonData, short x, short y);
	static void feed(char actionButtonId, char buttonData, short x, short y, short dx, short dy);

private:
	static MouseDevice _instance;
};

#endif//MOUSE_H__
