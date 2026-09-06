#ifndef MTH_H__
#define MTH_H__

#include <vector>
#include <set>
#include <algorithm>

namespace Mth {

	extern const float PI;
	extern const float TWO_PI;
	extern const float RADDEG;
	extern const float DEGRAD;

	void initMth();

	float sqrt(float x);
	float invSqrt(float x);

	int floor(float x);

	float sin(float x);
	float cos(float x);

	float atan(float x);
	float atan2(float dy, float dx);

	float random();
	int random(int n);

	float abs(float a);
	float Min(float a, float b);
	float Max(float a, float b);
	int abs(int a);
	int Min(int a, int b);
	int Max(int a, int b);

	int   clamp(int v, int low, int high);
	float clamp(float v, float low, float high);
	float lerp(float src, float dst, float alpha);
	int   lerp(int src, int dst, float alpha);

	///@param value The original signed value
	///@param with  The (possibly signed) value to "abs-decrease" <value> with
	///@param min   The minimum value
	float absDecrease(float value, float with, float min);
	//float absIncrease(float value, float with, float max);

	float absMax(float a, float b);
	float absMaxSigned(float a, float b);

	int intFloorDiv(int a, int b);
};

namespace Util
{
	template <class T>
	int removeAll(std::vector<T>& superset, const std::vector<T>& toRemove) {
		int subSize = (int)toRemove.size();
		int removed = 0;

		for (int i = 0; i < subSize; ++i) {
			T elem = toRemove[i];
			int size = (int)superset.size();
			for (int j = 0; j < size; ++j) {
				if (elem == superset[j]) {
					superset.erase( superset.begin() + j, superset.begin() + j + 1);
					++removed;
					break;
				}
			}
		}
		return removed;
	}

	template <class T>
	bool remove(std::vector<T>& list, const T& instance) {
		typename std::vector<T>::iterator it = std::find(list.begin(), list.end(), instance);
		if (it == list.end())
			return false;

		list.erase(it);
		return true;
	}

	// Could perhaps do a template<template ..>
	template <class T>
	bool remove(std::set<T>& list, const T& instance) {
		typename std::set<T>::iterator it = std::find(list.begin(), list.end(), instance);
		if (it == list.end())
			return false;

		list.erase(it);
		return true;
	}
};

#endif // MTH_H__
