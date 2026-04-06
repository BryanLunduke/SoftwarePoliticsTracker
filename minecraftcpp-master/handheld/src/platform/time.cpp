#include "time.h"
#include <cstring>

#ifdef WIN32
    #include <windows.h>
#else
	#include <sys/time.h>
	#include <unistd.h>
#endif

// Ugly ass temporary solution;
// since gettimeofday() returns seconds & microseconds since
// Epoch(1970) rather than since computer started, getTimeMs()
// would overflow if just using an int -> subtract the number
// of seconds that had passed when starting the app before
// doing any calculations.
#ifdef WIN32
	static long long getStartTime() {
		LARGE_INTEGER ts;
		QueryPerformanceCounter(&ts);
		return ts.QuadPart;
	}
	static unsigned long long _t_start = getStartTime();

#else
	static int getStartTime() {
		timeval now;
		gettimeofday(&now, 0);
		return now.tv_sec;
	}
	static unsigned int _t_start = getStartTime();
#endif

#include <cstdio>

/// Returns a platform dependent time in seconds
/// This is allowed to be here, since getTimeS and getTimeMs both have t(0)
/// as base (and obviously not good for an initial random seed)
int getRawTimeS() {
#ifdef WIN32
	return (int)(GetTickCount() / 1000);
#else
	timeval now;
	gettimeofday(&now, 0);
	return (int)now.tv_sec;
#endif
}

float getTimeS() {
#ifdef WIN32
	// If the game seems to go with different speeds, this might most
	// possibly be the cause. QueryPerformanceCounter has indeed high
	// resolution, but (imho) only guaranteed to be correct for (short)
	// deltas between consecutive calls
	LARGE_INTEGER freq, ts;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&ts);
	return (ts.QuadPart - _t_start) / (float)freq.QuadPart;
#else
    timeval now;
    gettimeofday(&now, 0);
    return (float)(now.tv_sec - _t_start) + now.tv_usec / 1000000.0f;
#endif
}

int getTimeMs() {
	return (int)(getTimeS() * 1000.0f);
}

void sleepMs(int ms) {
#ifdef WIN32
    Sleep(ms);
#else
	usleep(ms * 1000);
#endif
}

#include <ctime>
int getEpochTimeS() {
	return (int)std::time (NULL);
}


StopwatchNLast::StopwatchNLast( int numMeasurements )
:	n(numMeasurements),
	k(0),
	sum(0)
{
	t = new float[n];
	memset(t, 0, n * sizeof(float));
}

StopwatchNLast::~StopwatchNLast() {
	delete[] t;
}

float StopwatchNLast::stop() {
	float total = Stopwatch::stop();
	float curr  = Stopwatch::getLast();

	float toDelete = t[k];
	if (++k == n) k = 0;
	t[k] = curr;

	sum += (curr - toDelete);

	return total;
}

void StopwatchNLast::print( const std::string& prepend /*= ""*/ ) {
	LOGI("%s\tTime (AVGms for the last %d runs): %f (Max: %f)\n",
		prepend.c_str(), n, (1000.0f*sum/n), 1000.0f * getMax());
}


//
// Stopwatch
//
Stopwatch::Stopwatch()
:	_count(0),
	_printcounter(0)
{
	reset();
}

void Stopwatch::start() {
	_st = getTimeS();
}

float Stopwatch::stop() {
	if (_st == -1) return 0;
	_last = getTimeS() - _st;
	if (_last > _max) _max = _last;
	_tt += _last;
	_st = -1;
	++_count;
	return _tt;
}

float Stopwatch::stopContinue() {
	if (_st == -1) return 0;
	const float now = getTimeS();
	_last = now - _st;
	if (_last > _max) _max = _last;
	_tt += _last;
	_st = now;
	++_count;
	return _tt;
}

float Stopwatch::getLast() {
	return _last;
}

float Stopwatch::getTotal() {
	return _tt;
}

float Stopwatch::getMax() {
	return _max;
}

int Stopwatch::getCount() {
	return _count;
}

void Stopwatch::reset() {
	_st = -1;
	_tt = 0;
	_max = 0;
}

void Stopwatch::printEvery( int n, const std::string& prepend /*= ""*/ ) {
	if (++_printcounter >= n) {
		_printcounter = 0;
		print(prepend);
	}
}

void Stopwatch::print( const std::string& prepend /*= ""*/ ) {
	if (!_count) return;
	LOGI("%s\tTime (AVGms/LTs(MAXs)/TTs, C) : %.3f/%.6f(%.6f)/%.4f, %d\n",
		prepend.c_str(), (1000.0f*_tt/_count), _last, _max, _tt, _count);
}

//
// StopwatchHandler - a collection of StopWatch:es
//
Stopwatch& StopwatchHandler::get( const std::string& s ) {
	Map::iterator it = _map.find(s);
	if (it != _map.end())
		return *it->second;

	Stopwatch* watch = new Stopwatch;
	_map.insert(make_pair(s, watch));
	return *watch;
}

void StopwatchHandler::clear( const std::string& s ) {
	Map::iterator it = _map.find(s);
	if (it != _map.end()) {
		delete it->second;
	}
	_map.erase(s);
}

void StopwatchHandler::clearAll() {
	for (Map::iterator it = _map.begin(); it != _map.end(); ++it)
		delete it->second;
	_map.clear();
}

void StopwatchHandler::print() {
	for (Map::iterator it = _map.begin(); it != _map.end(); ++it) {
		it->second->print(it->first.c_str());
	}
}

void StopwatchHandler::printEvery( int n ) {
	if (++_printcounter >= n) {
		_printcounter = 0;
		print();
	}
}
