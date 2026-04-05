#include "Mouse.h"

//
// MouseAction
//
MouseAction::MouseAction(char actionButtonId, char buttonData, short x, short y, char pointerId)
{
	this->action = actionButtonId;
	this->data = buttonData;
	this->x = x;
	this->y = y;
	this->dx = this->dy = 0;
	this->pointerId = pointerId;
}

MouseAction::MouseAction(char actionButtonId, char buttonData, short x, short y, short dx, short dy, char pointerId)
{
	this->action = actionButtonId;
	this->data = buttonData;
	this->x = x;
	this->y = y;
	this->dx = dx;
	this->dy = dy;
	this->pointerId = pointerId;
}

bool MouseAction::isButton() const
{
	return action == ACTION_LEFT || action == ACTION_RIGHT;
}

//
// MouseDevice
//
MouseDevice::MouseDevice()
:	_index(-1),
	_x(0), _xOld(0),
	_y(0), _yOld(0),
	_dx(DELTA_NOTSET), _dy(DELTA_NOTSET),
	_firstMovementType(0)
{
	for (int i = 0; i < MAX_NUM_BUTTONS; ++i)
		_buttonStates[i] = 0;
}

void MouseDevice::reset() {
	_index = -1;
	_inputs.clear();
	_buttonStates[MouseAction::ACTION_WHEEL] = 0;
}

char MouseDevice::getButtonState(int buttonId) {
	if (buttonId < MouseAction::ACTION_LEFT || buttonId > MouseAction::ACTION_WHEEL)
		return 0;
	return _buttonStates[buttonId];
}

bool MouseDevice::isButtonDown(int buttonId) {
	return getButtonState(buttonId) != 0;
}

/// Was the current movement the first movement after mouse down?
bool MouseDevice::wasFirstMovement() {
	return _firstMovementType == 1;
}

short MouseDevice::getX() { return _x; }
short MouseDevice::getY() { return _y; }

short MouseDevice::getDX() { return (DELTA_NOTSET != _dx)? _dx : _x - _xOld; }
short MouseDevice::getDY() { return (DELTA_NOTSET != _dy)? _dy : _y - _yOld; }

void MouseDevice::reset2() {
	_xOld = _x;
	_yOld = _y;
	_dx = _dy = DELTA_NOTSET;
}

bool MouseDevice::next() {
	if (_index + 1 >= (int)_inputs.size())
		return false;

	++_index;
	return true;
}

void MouseDevice::rewind() {
	_index = -1;
}

bool MouseDevice::getEventButtonState() {
	return _inputs[_index].data == MouseAction::DATA_DOWN;
}

char MouseDevice::getEventButton() {
	return _inputs[_index].action;
}

const MouseAction& MouseDevice::getEvent() { return _inputs[_index]; }

void MouseDevice::feed(char actionButtonId, char buttonData, short x, short y) {
	feed(actionButtonId, buttonData, x, y, 0, 0);
}

void MouseDevice::feed(char actionButtonId, char buttonData, short x, short y, short dx, short dy) {

	_inputs.push_back(MouseAction(actionButtonId, buttonData, x, y, dx, dy, 0));

	if (actionButtonId != MouseAction::ACTION_MOVE) {
		_buttonStates[actionButtonId] = buttonData;

		if (actionButtonId == MouseAction::ACTION_LEFT)
			_firstMovementType = -1;
	} else {
		if (_dx == DELTA_NOTSET) {
			_dx = _dy = 0;
		}
		_dx += dx;
		_dy += dy;

		if (_firstMovementType == -1)
			_firstMovementType = 1;
		else
			_firstMovementType = 0;
	}

	_xOld = _x;
	_yOld = _y;
	_x = x;
	_y = y;
}

//
// Mouse - static class wrapping a MouseDevice
//
void Mouse::reset() { _instance.reset(); }

char Mouse::getButtonState(int buttonId) { return _instance.getButtonState(buttonId); }

bool Mouse::isButtonDown(int buttonId) { return _instance.isButtonDown(buttonId); }

short Mouse::getX() { return _instance.getX(); }
short Mouse::getY() { return _instance.getY(); }

short Mouse::getDX() { return _instance.getDX(); }
short Mouse::getDY() { return _instance.getDY(); }

void Mouse::reset2() { _instance.reset2(); }

bool Mouse::next() {  return _instance.next(); }
void Mouse::rewind() { _instance.rewind(); }

bool Mouse::getEventButtonState() { return _instance.getEventButtonState(); }

char Mouse::getEventButton() { return _instance.getEventButton(); }

const MouseAction& Mouse::getEvent() { return _instance.getEvent(); }

void Mouse::feed(char actionButtonId, char buttonData, short x, short y) {
	feed(actionButtonId, buttonData, x, y, 0, 0);
}
void Mouse::feed(char actionButtonId, char buttonData, short x, short y, short dx, short dy) {
	//LOGI("Mouse::feed: %d, %d, xy: %d, %d\n", actionButtonId, buttonData, x, y);
	return _instance.feed(actionButtonId, buttonData, x, y, dx, dy);
}


MouseDevice Mouse::_instance;

