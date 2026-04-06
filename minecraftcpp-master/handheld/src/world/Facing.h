#ifndef NET_MINECRAFT__Facing_H__
#define NET_MINECRAFT__Facing_H__

//package net.minecraft;

class Facing
{
public:
	static const int DOWN = 0;
	static const int UP = 1;
	static const int NORTH = 2;
	static const int SOUTH = 3;
	static const int WEST = 4;
	static const int EAST = 5;
    
    static const char* toString(int face) {
        if (face == DOWN)  return "Down";
        if (face == UP  )  return "Up";
        if (face == NORTH) return "North";
        if (face == SOUTH) return "South";
        if (face == WEST)  return "West";
        if (face == EAST)  return "East";
        return "Unknown facing";
    }

	// implemented in Direction.cpp

	static const int OPPOSITE_FACING[6];
	static const int STEP_X[6];
	static const int STEP_Y[6];
	static const int STEP_Z[6];
};

#endif /*NET_MINECRAFT__Facing_H__*/
