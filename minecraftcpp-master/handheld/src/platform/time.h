#ifndef TIME_H__
#define TIME_H__

#include <string>
#include <map>
#include "log.h"

float getTimeS();
int getTimeMs();
int getRawTimeS();
int getEpochTimeS();

void sleepMs(int ms);

class Stopwatch {
public:
    Stopwatch();
    virtual ~Stopwatch() {}

    void start();
	/** Returns the total number of seconds and stops the clock */
    virtual float stop();
	/* Returns the total number of seconds, but wont stop the clock */
	virtual float stopContinue();
	/** Returns the last measured time interval in seconds */
	float getLast();
	/** Returns the total measured time in seconds */
	float getTotal();
	/** Returns the maximum time between start() and stop() */
	float getMax();
	/** Returns the number of times the watch has been started and stopped */
	int getCount();

	void reset();
	void printEvery(int n, const std::string& prepend = "");
	virtual void print(const std::string& prepend = "");
private:
    float _st;
    float _tt;
	float _last;
	float _max;
	int _count;
	int _printcounter;
};

class SwStartStopper {
public:
	SwStartStopper(Stopwatch& stopwatch)
	:   _stopwatch(stopwatch)
	{
		_stopwatch.start();
	}

	~SwStartStopper() {
		_stopwatch.stop();
	}
private:
	Stopwatch& _stopwatch;
};

class SwStopper {
public:
	SwStopper(Stopwatch& stopwatch)
	:   _stopwatch(stopwatch)
	{}

	~SwStopper() {
		_stopwatch.stop();
	}
private:
	Stopwatch& _stopwatch;
};

class StopwatchHandler {
typedef std::map<std::string, Stopwatch*> Map;
public:
    StopwatchHandler()
	:	_printcounter(0)
	{}
	~StopwatchHandler() {
		clearAll();
    }
	Stopwatch& get(const std::string& s);
	void clear(const std::string& s);
	void clearAll();
	void print();
	void printEvery(int n);

private:
    Map _map;
	int _printcounter;
};

class StopwatchNLast: public Stopwatch {
public:
	StopwatchNLast(int numMeasurements);
	~StopwatchNLast();

	float stop();

	virtual void print(const std::string& prepend = "");
private:
	int n;
	int k;
	float* t;
	float sum;

	Stopwatch sw;
};

#endif /*TIME_H__*/
