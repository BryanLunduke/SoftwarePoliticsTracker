#ifndef NET_MINECRAFT_CLIENT_PLAYER__TouchAreaModel_H__
#define NET_MINECRAFT_CLIENT_PLAYER__TouchAreaModel_H__

#include "ITouchScreenModel.h"
#include <vector>
#include "../../../../platform/time.h"

/// Interface for an area that can be point tested
class IArea{
public:
	IArea()
	:	deleteMe(true)
	{}

	virtual ~IArea() {};
	virtual bool isInside(float x, float y) = 0;
	//virtual void expandRectToInclude(float& l, float& t, float& r, float& b) {}

	bool deleteMe;
};

/// Holder for multiple <IArea, int areaId> pairs
class TouchAreaModel: public ITouchScreenModel
{
public:
	TouchAreaModel() {}

	~TouchAreaModel() {
		clear();
	}

	void clear() {
		for (unsigned int i = 0; i < _areas.size(); ++i) {
			Area* a = _areas[i];
			if (a->area && a->area->deleteMe)
				delete a->area;
			delete a;
		}
		_areas.clear();
	}

	int getPointerId(const MouseAction& m) {
		return getPointerId(m.x, m.y, m.pointerId);
	}
	
	int getPointerId(int x, int y, int pid) {
		//static Stopwatch w;
		//w.printEvery(200, "IArea");
		//w.start();
		float fx = (float)x, fy = (float) y;
		for (unsigned int i = 0; i < _areas.size(); ++i) {
			const Area* a = _areas[i];
			if (a->area->isInside(fx, fy)) {
				//w.stop();
				return a->areaId;
			}
		}
		//w.stop();
		return pid;
	}

	void addArea(int areaId, IArea* area) {
		Area* a = new Area();
		a->area = area;
		a->areaId = areaId;

		_areas.push_back( a );
	}

private:
	struct Area {
		IArea* area;
		int areaId;
	};
	
	std::vector<Area*> _areas;
};

/// Area represented by a polygon
class PolygonArea: public IArea
{
public:
	PolygonArea(int numPoints, const float* xp, const float* yp)
	:	_numPoints(numPoints)
	{
		_x = new float[numPoints];
		_y = new float[numPoints];

		for (int i = 0; i < numPoints; ++i) {
			_x[i] = xp[i];
			_y[i] = yp[i];
		}
	}

	~PolygonArea() {
		delete[] _x;
		delete[] _y;
	}

    virtual bool isInside(float x, float y)
    {
	  bool c = false;
      for (int i = 0, j = _numPoints-1; i < _numPoints; j = i++) {
        if ((((_y[i] <= y) && (y < _y[j])) ||
             ((_y[j] <= y) && (y < _y[i]))) &&
            (x < (_x[j] - _x[i]) * (y - _y[i]) / (_y[j] - _y[i]) + _x[i]))
          c = !c;
      }
      return c == 1;
    }

//private:
	float* _x;
	float* _y;
	int _numPoints;
};

/// Area represented by a rectangle
class RectangleArea: public IArea
{
public:
	RectangleArea(float x0, float y0, float x1, float y1)
	:	_x0(x0),
		_x1(x1),
		_y0(y0),
		_y1(y1)
	{}

	virtual bool isInside(float x, float y) {
		return x >= _x0 && x <= _x1
			&& y >= _y0 && y <= _y1;
	}

	virtual float centerX()
	{
		return _x0 + (_x1 - _x0) * .5f;
	}

	virtual float centerY()
	{
		return _y0 + (_y1 - _y0) * .5f;
	}

//private:
	float _x0, _x1;
	float _y0, _y1;
};

/// An area represented by a circle
class CircleArea: public IArea
{
public:
	CircleArea(float x, float y, float r)
	:	_x(x), _y(y), _rr(r*r)
	{}

	virtual bool isInside(float x, float y) {
		const float dx = x - _x;
		const float dy = y - _y;
		return (dx*dx + dy*dy) <= _rr;
	}

private:
	float _x, _y;
	float _rr;
};

/// Inside any area in include list but none of exclude list
class IncludeExcludeArea: public IArea
{
public:
	IncludeExcludeArea() {}

	~IncludeExcludeArea() {
		clear();
	}

	void clear() {
		if (deleteMe) {
			for (unsigned int i = 0; i < _includes.size(); ++i) {
				IArea* area = _includes[i];
				if (area->deleteMe) delete area;
			}
			for (unsigned int i = 0; i < _excludes.size(); ++i) {
				IArea* area = _excludes[i];
				if (area->deleteMe) delete area;
			}
		}
		_includes.clear();
		_excludes.clear();
	}

	void include(IArea* area) { _includes.push_back(area); }
	void exclude(IArea* area) { _excludes.push_back(area); }

	virtual bool isInside(float x, float y) {
		for (unsigned int i = 0; i < _includes.size(); ++i)
			if (_includes[i]->isInside(x, y)) {
				bool good = true;
				//@todo: cache _exclude-test results
				for (unsigned int j = 0; j < _excludes.size(); ++j)
					if (_excludes[j]->isInside(x, y)) {
						good = false;
						break;
					}
				if (good) {
					return true;
				}
			}
		return false;
	}

private:
	std::vector<IArea*> _includes;
	std::vector<IArea*> _excludes;
};


/// At least one of the areas
class OrArea: public IArea
{
public:
	OrArea(IArea* a, IArea* b)
	:	_a(a),
		_b(b)
	{}

	~OrArea() {
		if (deleteMe) {
			if (_a->deleteMe) delete _a;
			if (_b->deleteMe) delete _b;
		}
	}

	virtual bool isInside(float x, float y) {
		return _a->isInside(x, y) || _b->isInside(x, y);
	}

private:
	IArea* _a;
	IArea* _b;
};

/// In both areas
class AndArea: public IArea
{
public:
	AndArea(IArea* a, IArea* b)
	:	_a(a),
		_b(b)
	{}

	~AndArea() {
		if (deleteMe) {
			if (_a->deleteMe) delete _a;
			if (_b->deleteMe) delete _b;
		}
	}

	virtual bool isInside(float x, float y) {
		return _a->isInside(x, y) && _b->isInside(x, y);
	}

private:
	IArea* _a;
	IArea* _b;
};

/// Exactly one of the areas
class XorArea: public IArea
{
public:
	XorArea(IArea* a, IArea* b)
	:	_a(a),
		_b(b)
	{}

	~XorArea() {
		if (deleteMe) {
			if (_a->deleteMe) delete _a;
			if (_b->deleteMe) delete _b;
		}
	}

	virtual bool isInside(float x, float y) {
		return _a->isInside(x, y) ^ _b->isInside(x, y);
	}

private:
	IArea* _a;
	IArea* _b;
};

/// Everything except this area
class NotArea: public IArea
{
public:
	NotArea(IArea* a)
	:	_a(a)
	{}

	~NotArea() {
		if (deleteMe && _a->deleteMe) delete _a;
	}

	virtual bool isInside(float x, float y) {
		return !_a->isInside(x, y);
	}

private:
	IArea* _a;
};

/// First area, but not second
/// This is semantically the same as AndArea( a, new NotArea(b) )
class DifferenceArea: public IArea
{
public:
	DifferenceArea(IArea* a, IArea* b)
	:	_a(a),
		_b(b)
	{}

	~DifferenceArea() {
		if (deleteMe) {
			if (_a->deleteMe) delete _a;
			if (_b->deleteMe) delete _b;
		}
	}

	virtual bool isInside(float x, float y) {
		return _a->isInside(x, y) && !_b->isInside(x, y);
	}
private:
	IArea* _a;
	IArea* _b;
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER__TouchAreaModel_H__*/
