#include "Multitouch.h"

int
	Multitouch::_index = -1,
	Multitouch::_activePointerCount = 0,
	Multitouch::_activePointerList[Multitouch::MAX_POINTERS] = {-1},
	Multitouch::_activePointerThisUpdateCount = 0,
	Multitouch::_activePointerThisUpdateList[Multitouch::MAX_POINTERS] = {-1};

bool
	Multitouch::_wasPressed[Multitouch::MAX_POINTERS] = {false},
	Multitouch::_wasReleased[Multitouch::MAX_POINTERS] = {false},
	Multitouch::_wasPressedThisUpdate[Multitouch::MAX_POINTERS] = {false},
	Multitouch::_wasReleasedThisUpdate[Multitouch::MAX_POINTERS] = {false};

TouchPointer
    Multitouch::_pointers[Multitouch::MAX_POINTERS];

std::vector<MouseAction>
    Multitouch::_inputs; 
