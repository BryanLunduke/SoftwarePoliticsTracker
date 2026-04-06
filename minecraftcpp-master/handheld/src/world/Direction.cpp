#include "Direction.h"
#include "Facing.h"

const int Direction::DIRECTION_FACING[4] = {
	Facing::SOUTH,
	Facing::WEST,
	Facing::NORTH,
	Facing::EAST
};
const int Direction::FACING_DIRECTION[6] = {
	Direction::UNDEFINED,
	Direction::UNDEFINED,
	Direction::NORTH,
	Direction::SOUTH,
	Direction::WEST,
	Direction::EAST
};
const int Direction::DIRECTION_OPPOSITE[4] = {
	Direction::NORTH,
	Direction::EAST,
	Direction::SOUTH,
	Direction::WEST
};

const int Direction::RELATIVE_DIRECTION_FACING[4][6] = {
	// south
    {	Facing::DOWN,
		Facing::UP,
		Facing::SOUTH,
		Facing::NORTH,
		Facing::EAST,
		Facing::WEST },
	// west
	{	Facing::DOWN,
		Facing::UP,
		Facing::EAST,
		Facing::WEST,
		Facing::NORTH,
		Facing::SOUTH },
    // north
	{	Facing::DOWN,
		Facing::UP,
		Facing::NORTH,
		Facing::SOUTH,
		Facing::WEST,
		Facing::EAST },
    // east
	{	Facing::DOWN,
		Facing::UP,
		Facing::WEST,
		Facing::EAST,
		Facing::SOUTH,
		Facing::NORTH }
};

// declared in Facing.h

const int Facing::OPPOSITE_FACING[6] = {
	Facing::UP, Facing::DOWN, Facing::SOUTH, Facing::NORTH, Facing::EAST, Facing::WEST
};

const int Facing::STEP_X[6] = {
	0, 0, 0, 0, -1, 1
};
const int Facing::STEP_Y[6] = {
	-1, 1, 0, 0, 0, 0
};
const int Facing::STEP_Z[6] = {
	0, 0, -1, 1, 0, 0
};

