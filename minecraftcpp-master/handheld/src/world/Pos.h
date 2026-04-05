#ifndef NET_MINECRAFT_WORLD_Pos_H__
#define NET_MINECRAFT_WORLD_Pos_H__

//package net.minecraft;

#include <sstream>

class Pos
{
public:
    Pos()
	:	x(0),
		y(0),
		z(0)
    {}

    Pos(int x, int y, int z)
	:	x(x),
		y(y),
		z(z)
	{}

    Pos(const Pos& position)
	:	x(position.x),
		y(position.y),
		z(position.z)
	{}

    __inline bool operator==(const Pos& rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

	bool operator<(const Pos& rhs) const {
		return compareTo(rhs) < 0;
	}

	static int createHashCode(int x, int y, int z) {
		return x + (z << 8) + (y << 16);
	}
    int hashCode() const {
        return x + (z << 8) + (y << 16);
    }

    int compareTo(const Pos& pos) const {
		return hashCode() - pos.hashCode();
    }

    Pos offset(int x, int y, int z) const {
        return Pos(this->x + x, this->y + y, this->z + z);
    }

    void set(int x, int y, int z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    void set(const Pos& pos) {
        x = pos.x;
        y = pos.y;
        z = pos.z;
    }

    Pos above() const {
        return Pos(x, y + 1, z);
    }

    Pos above(int steps) const {
        return Pos(x, y + steps, z);
    }

    Pos below() const {
        return Pos(x, y - 1, z);
    }

    Pos below(int steps) const {
        return Pos(x, y - steps, z);
    }

    Pos north() const {
        return Pos(x, y, z - 1);
    }

    Pos north(int steps) const {
        return Pos(x, y, z - steps);
    }

    Pos south() const {
        return Pos(x, y, z + 1);
    }

    Pos south(int steps) const {
        return Pos(x, y, z + steps);
    }

    Pos west() const {
        return Pos(x - 1, y, z);
    }

    Pos west(int steps) const {
        return Pos(x - 1, y, z);
    }

    Pos east() const {
        return Pos(x + 1, y, z);
    }

    Pos east(int steps) const {
        return Pos(x + steps, y, z);
    }

    void move(int x, int y, int z) {
        this->x += x;
        this->y += y;
        this->z += z;
    }

    void move(const Pos& pos) {
        x += pos.x;
        y += pos.y;
        z += pos.z;
    }

    void moveX(int steps) {
        x += steps;
    }

    void moveY(int steps) {
        y += steps;
    }

    void moveZ(int steps) {
        z += steps;
    }

    void moveUp(int steps) {
        y += steps;
    }

    void moveUp() {
        ++y;
    }

    void moveDown(int steps) {
        y -= steps;
    }

    void moveDown() {
        --y;
    }

    void moveEast(int steps) {
        x += steps;
    }

    void moveEast() {
        ++x;
    }

    void moveWest(int steps) {
        x -= steps;
    }

    void moveWest() {
        --x;
    }

    void moveNorth(int steps) {
        z -= steps;
    }

    void moveNorth() {
        --z;
    }

    void moveSouth(int steps) {
        z += steps;
    }

	void moveSouth() {
		++z;
	}

    std::string toString() const {
		std::stringstream ss;
		ss << "Pos(" << x << "," << y << "," << z << ")";
		return ss.str();
    }

    int x, y, z;
};

#endif /*NET_MINECRAFT_WORLD_Pos_H__*/
