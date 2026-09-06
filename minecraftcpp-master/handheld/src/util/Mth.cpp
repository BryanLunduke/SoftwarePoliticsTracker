#include "Mth.h"
#include "Random.h"
#include <cmath>
#include <vector>

#include "../Performance.h"

static Random _rand;

namespace Mth
{
	const float PI = 3.1415926535897932384626433832795028841971f; // exactly!
	const float TWO_PI = 2.0f * PI; // exactly!
	const float DEGRAD = PI / 180.0f;
	const float RADDEG = 180.0f / PI;

	static float _sin[65536];
	static const float _sinScale = 65536.0f / (2.0f * PI);

	void initMth() {
		for (int i = 0; i < 65536; ++i)
			_sin[i] = ::sin(i / _sinScale);
	}

	float sqrt(float x) {
		//Stopwatch& w = Performance::watches.get("sqrt");
		//w.start();
			float ret = ::sqrt(x);
		//w.stop();
		return ret;
	}

	static __inline float fastInvSqrt(float x) {
		float xhalf = 0.5f*x;
		int i = *(int*)&x;
		i = 0x5f3759df - (i>>1);
		x = *(float*)&i;
		x = x*(1.5f - xhalf*x*x);
		return x;
	}
	float invSqrt(float x) {
		//Stopwatch& w = Performance::watches.get("invSqrt");
		//w.start();
		float ret = fastInvSqrt(x);//1.0f / sqrt(x);
		//w.stop();
		return ret;
	}

	int floor(float v) {
		int i = (int) v;
		return v < i ? i - 1 : i;
	}

	float cos(float x) {
		//Performance::watches.get("cos").start();
		//float ret = ::cos(x);
		float ret = _sin[(int) (x * _sinScale + 65536 / 4) & 65535];
		//Performance::watches.get("cos").stop();
		return ret;
	}

	float sin(float x) {
		//Performance::watches.get("sin").start();
		//float ret = ::sin(x);
		float ret = _sin[(int) (x * _sinScale) & 65535];
		//Performance::watches.get("sin").stop();
		return ret;
	}

	float atan(float x) {
		//Performance::watches.get("atan").start();
		float ret = ::atan(x);
		//Performance::watches.get("atan").stop();
		return ret;
	}

	float atan2(float dy, float dx) {
		//Performance::watches.get("atan2").start();
		float ret = ::atan2(dy, dx);
		//Performance::watches.get("atan2").stop();
		return ret;
	}

	float random(){
		return _rand.nextFloat();
	}
	int random(int n){
		return _rand.nextInt(n);
	}

    int intFloorDiv(int a, int b) {
        if (a < 0) return -((-a - 1) / b) - 1;
        return a / b;
    }

	float abs(float a) { return a>=0? a : -a; }
	float Min(float a, float b) { return a<=b? a : b; }
	float Max(float a, float b) { return a>=b? a : b; }

	int abs(int a) { return a>=0? a : -a; }
	int Min(int a, int b) { return a<=b? a : b; }
	int Max(int a, int b) { return a>=b? a : b; }

	float absDecrease(float value, float with, float min) {
		if (value > 0) return Max(min, value - with);
		return Min(value + with, -min);
	}
	//float absIncrease(float value, float with, float max);

	int clamp(int v, int low, int high) {
		if (v > high) return high;
		return v>low? v : low;
	}
	float clamp(float v, float low, float high) {
		if (v > high) return high;
		return v>low? v : low;
	}
	float lerp(float src, float dst, float alpha) {
		return src + (dst - src) * alpha;
	}
	int lerp(int src, int dst, float alpha) {
		return src + (int)((dst - src) * alpha);
	}

    float absMax(float a, float b) {
        if (a < 0) a = -a;
        if (b < 0) b = -b;
        return a > b ? a : b;
    }

	float absMaxSigned(float a, float b) {
		return abs(a) > abs(b)? a : b;
	}
};
