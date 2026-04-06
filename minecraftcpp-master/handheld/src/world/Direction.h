#ifndef NET_MINECRAFT__Direction_H__
#define NET_MINECRAFT__Direction_H__

//package net.minecraft;

class Direction
{
public:
	static const int UNDEFINED = -1;
    static const int SOUTH	= 0;
    static const int WEST	= 1;
    static const int NORTH	= 2;
    static const int EAST	= 3;

    // for [direction] it gives [tile-face]
    static const int DIRECTION_FACING[4];

	// for a given face gives direction
	static const int FACING_DIRECTION[6];

	// for [direction] it gives [opposite direction]
    static const int DIRECTION_OPPOSITE[4];

	// for [direction][world-facing] it gives [tile-facing]
    static const int RELATIVE_DIRECTION_FACING[4][6];
};

#endif /*NET_MINECRAFT__Direction_H__*/
