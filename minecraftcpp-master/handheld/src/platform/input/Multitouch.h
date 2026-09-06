#ifndef MULTITOUCH_H__
#define MULTITOUCH_H__

#include "Mouse.h"
#include "../log.h"

typedef MouseDevice TouchPointer;

class Multitouch
{
public:
	static const int MAX_POINTERS = 12;

	static bool isPointerDown(int pointerId) {
		return g(pointerId).isButtonDown(1);
	}

	// Returns the first activeExtended (down OR released) pointer id
	static int getFirstActivePointerIdEx() {
		for (int i = 0; i < MAX_POINTERS; ++i)
			if (_pointers[i].isButtonDown(1)) return i;
		for (int i = 0; i < MAX_POINTERS; ++i)
			if (_wasReleased[i]) return i;
		return -1;
	}

	// Returns the first activeExtended (down OR released) pointer id
	// Compared to getFirstActivePointerIdEx, this method keeps the state the
	// tick/render loop, while the former keeps it for a tick.
	static int getFirstActivePointerIdExThisUpdate() {
		for (int i = 0; i < MAX_POINTERS; ++i)
			if (_pointers[i].isButtonDown(1)) return i;
		for (int i = 0; i < MAX_POINTERS; ++i)
			if (_wasReleasedThisUpdate[i]) return i;
		return -1;
	}

	// Get a list of all active (i.e. marked as touched/down) pointer ids
	static int getActivePointerIds(const int** const ids) {
		*ids = _activePointerList;
		return _activePointerCount;
	}

	// Get a list of all active (i.e. marked as touched/down) pointer ids
	static int getActivePointerIdsThisUpdate(const int** const ids) {
		*ids = _activePointerThisUpdateList;
		return _activePointerThisUpdateCount;
	}

	// Called when no more events will be pushed this update
	static void commit() {
		_activePointerCount = 0;
		_activePointerThisUpdateCount = 0;
		for (int i = 0; i < MAX_POINTERS; ++i) {
			bool isDown = _pointers[i].isButtonDown(1);
			if (isDown) {
				_activePointerList[_activePointerCount++] = i;
				_activePointerThisUpdateList[_activePointerThisUpdateCount++] = i;
			} else if (_wasReleased[i]) {
				_activePointerThisUpdateList[_activePointerThisUpdateCount++] = i;
			}
		}
	}

	static short getX(int pointerId) { return g(pointerId).getX(); }
	static short getY(int pointerId) { return g(pointerId).getY(); }

	static short getDX(int pointerId) { return g(pointerId).getDX(); }
	static short getDY(int pointerId) { return g(pointerId).getDY(); }

	static bool wasFirstMovement(int pointerId) { return g(pointerId).wasFirstMovement(); }

	static bool isPressed(int pointerId) {
		return _wasPressed[_clampPointerId(pointerId)];
	}
	static bool isReleased(int pointerId) {
		return _wasReleased[_clampPointerId(pointerId)];
	}
	static bool isPressedThisUpdate(int pointerId) {
		return _wasPressedThisUpdate[_clampPointerId(pointerId)];
	}
	static bool isReleasedThisUpdate(int pointerId) {
		return _wasReleasedThisUpdate[_clampPointerId(pointerId)];
	}

	static void reset() {
		//LOGI("mtouch is reset\n");
		_inputs.clear();
		_index = -1;

		for (int i = 0; i < MAX_POINTERS; ++i) {
			g(i).reset();

			_wasPressed[i] = false;
			_wasReleased[i] = false;
		}
	}
	static void resetThisUpdate() {
		for (int i = 0; i < MAX_POINTERS; ++i) {
			_wasPressedThisUpdate[i] = false;
			_wasReleasedThisUpdate[i] = false;
		}
	}

	static bool next() {
        if (_index + 1 >= (int)_inputs.size())
            return false;

        ++_index;
        return true;
    }

	static void rewind() {
		_index = -1;
	}

	static MouseAction& getEvent() {
		return _inputs[_index];
	}

	static void feed(char actionButtonId, char buttonData, short x, short y, char pointerId) {
		pointerId = _clampPointerId(pointerId);

		_inputs.push_back(MouseAction(actionButtonId, buttonData, x, y, pointerId));
		g(pointerId).feed(actionButtonId, buttonData, x, y);

		if (actionButtonId > 0) {
			if (buttonData == MouseAction::DATA_DOWN) {
				_wasPressed[pointerId] = true;
				_wasPressedThisUpdate[pointerId] = true;
				//LOGI("isPressed %d %d\n", pointerId, isPressed(pointerId));
			}
			else if (buttonData == MouseAction::DATA_UP) {
				_wasReleased[pointerId] = true;
				_wasReleasedThisUpdate[pointerId] = true;
				//LOGI("isReleased %d %d\n", pointerId, isReleased(pointerId));
			}
		}
	}

	static int getMaxPointers() {
		return MAX_POINTERS;
	}

private:
	static TouchPointer& g(int pointerId) {
		return _pointers[_clampPointerId(pointerId)];
	}

	static int _clampPointerId(int p) {
		if (p < 0) return p;
		if (p >= MAX_POINTERS) return MAX_POINTERS-1;
		return p;
	}

	// Between calls to reset()
	static bool _wasPressed[MAX_POINTERS];
	static bool _wasReleased[MAX_POINTERS];

	// Between calls to resetThisUpdate()
	static bool _wasPressedThisUpdate[MAX_POINTERS];
	static bool _wasReleasedThisUpdate[MAX_POINTERS];

	static TouchPointer _pointers[MAX_POINTERS];
	static std::vector<MouseAction> _inputs;

	static int _activePointerList[MAX_POINTERS];
	static int _activePointerCount;

	static int _activePointerThisUpdateList[MAX_POINTERS];
	static int _activePointerThisUpdateCount;

	static int _index;
};

#endif /*MULTITOUCH_H__*/
