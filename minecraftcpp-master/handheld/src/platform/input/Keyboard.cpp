#include "Keyboard.h"

/*
const int KeyboardAction::KEYUP = 0;
const int KeyboardAction::KEYDOWN = 1;
*/

int
	Keyboard::_states[256] = {0};

std::vector<KeyboardAction>
	Keyboard::_inputs;
std::vector<char>
	Keyboard::_inputText;

int	
	Keyboard::_index = -1;

int	
	Keyboard::_textIndex = -1;