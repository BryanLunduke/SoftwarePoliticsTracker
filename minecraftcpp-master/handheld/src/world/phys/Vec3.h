#ifndef NET_MINECRAFT_WORLD_PHYS__Vec3_H__
#define NET_MINECRAFT_WORLD_PHYS__Vec3_H__

//package net.minecraft.world.phys;

#include <cmath>
#include <string>
#include <sstream>

class Vec3
{
public:
	Vec3()
	:	x(0),
		y(0),
		z(0)
	{}

    Vec3(float x, float y, float z) {
        if (x == -0.0f) x = 0.0f;
        if (y == -0.0f) y = 0.0f;
        if (z == -0.0f) z = 0.0f;
        this->x = x;
        this->y = y;
        this->z = z;
    }

    Vec3* set(float x, float y, float z) {
        this->x = x;
        this->y = y;
        this->z = z;
        return this;
    }

	Vec3 operator+(const Vec3& rhs) const {
		return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
	}
	Vec3& operator+=(const Vec3& rhs) {
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}

	Vec3 operator-(const Vec3& rhs) const {
		return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
	}
	Vec3& operator-=(const Vec3& rhs) {
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}

    Vec3 operator*(float k) const {
        return Vec3(x * k, y * k, z * k);
    }
    Vec3& operator*=(float k) {
        x *= k;
        y *= k;
        z *= k;
        return *this;
    }

    Vec3 normalized() const {
        float dist = sqrt(x * x + y * y + z * z);
        if (dist < 0.0001f) return Vec3();
        return Vec3(x / dist, y / dist, z / dist); // newTemp
    }

    float dot(const Vec3& p) const {
        return x * p.x + y * p.y + z * p.z;
    }

    Vec3 cross(const Vec3& p) const {
        return Vec3(y * p.z - z * p.y, z * p.x - x * p.z, x * p.y - y * p.x); // newTemp
    }

    Vec3 add(float x, float y, float z) const {
        return Vec3(this->x + x, this->y + y, this->z + z); // newTemp
    }
	Vec3& addSelf(float x, float y, float z) {
		this->x += x;
		this->y += y;
		this->z += z;
		return *this;
	}
    Vec3 sub(float x, float y, float z) const {
        return Vec3(this->x - x, this->y - y, this->z - z); // newTemp
    }
	Vec3& subSelf(float x, float y, float z) {
		this->x -= x;
		this->y -= y;
		this->z -= z;
		return *this;
	}

	void negate() {
		x = -x;
		y = -y;
		z = -z;
	}

	Vec3 negated() const {
		return Vec3(-x, -y, -z);
	}

    float distanceTo(const Vec3& p) const {
        float xd = p.x - x;
        float yd = p.y - y;
        float zd = p.z - z;
        return sqrt(xd * xd + yd * yd + zd * zd);
    }

    float distanceToSqr(const Vec3& p) const {
        float xd = p.x - x;
        float yd = p.y - y;
        float zd = p.z - z;
        return xd * xd + yd * yd + zd * zd;
    }

    float distanceToSqr(float x2, float y2, float z2) const {
        float xd = x2 - x;
        float yd = y2 - y;
        float zd = z2 - z;
        return xd * xd + yd * yd + zd * zd;
    }

    float length() const {
        return sqrt(x * x + y * y + z * z);
    }

    bool clipX(const Vec3& b, float xt, Vec3& result) const {
        float xd = b.x - x;
        float yd = b.y - y;
        float zd = b.z - z;

        if (xd * xd < 0.0000001f) return false;

        float d = (xt - x) / xd;
        if (d < 0 || d > 1) return false;

		result.set(x + xd * d, y + yd * d, z + zd * d);
		return true;
    }

    bool clipY(const Vec3& b, float yt, Vec3& result) const {
        float xd = b.x - x;
        float yd = b.y - y;
        float zd = b.z - z;

        if (yd * yd < 0.0000001f) return false;

        float d = (yt - y) / yd;
        if (d < 0 || d > 1) return false;

		result.set(x + xd * d, y + yd * d, z + zd * d);
		return true;
    }

    bool clipZ(const Vec3& b, float zt, Vec3& result) const {
        float xd = b.x - x;
        float yd = b.y - y;
        float zd = b.z - z;

        if (zd * zd < 0.0000001f) return false;

        float d = (zt - z) / zd;
        if (d < 0 || d > 1) return false;

		result.set(x + xd * d, y + yd * d, z + zd * d);
		return true;
    }

    std::string toString() const {
		std::stringstream ss;
		ss << "Vec3(" << x << "," << y << "," << z << ")";
		return ss.str();
    }

    Vec3 lerp(const Vec3& v, float a) const {
        return Vec3(x + (v.x - x) * a, y + (v.y - y) * a, z + (v.z - z) * a); // newTemp
    }

    void xRot(float degs) {
        float cosv = cos(degs);
        float sinv = sin(degs);

        float xx = x;
        float yy = y * cosv + z * sinv;
        float zz = z * cosv - y * sinv;

        this->x = xx;
        this->y = yy;
        this->z = zz;
    }

    void yRot(float degs) {
        float cosv = cos(degs);
        float sinv = sin(degs);

        float xx = x * cosv + z * sinv;
        float yy = y;
        float zz = z * cosv - x * sinv;

        this->x = xx;
        this->y = yy;
        this->z = zz;
    }

    void zRot(float degs) {
        float cosv = cos(degs);
        float sinv = sin(degs);

        float xx = x * cosv + y * sinv;
        float yy = y * cosv - x * sinv;
        float zz = z;

        this->x = xx;
        this->y = yy;
        this->z = zz;
    }

	static Vec3 fromPolarXY(float angle, float radius) {
		return Vec3( radius * cos(angle), radius * sin(angle), 0 );
	}

	float x, y, z;
};

#endif /*NET_MINECRAFT_WORLD_PHYS__Vec3_H__*/
