#include "PerfTimer.h"
#include "../platform/time.h"
#include <algorithm>

/*static*/
bool
	PerfTimer::enabled = false;

std::vector<std::string>
	PerfTimer::paths;

std::vector<float>
	PerfTimer::startTimes;

std::string
	PerfTimer::path;

PerfTimer::TimeMap
	PerfTimer::times;


/*static*/
void PerfTimer::reset()
{
	times.clear();
}

/*static*/
void PerfTimer::push( const std::string& name )
{
	if (!enabled) return;
	if (path.length() > 0) path += ".";
	path += name;
	paths.push_back(path);
	startTimes.push_back(getTimeS());
}

/*static*/
void PerfTimer::pop()
{
	if (!enabled) return;
	float endTime = getTimeS();
	float startTime = startTimes.back();

	paths.pop_back();
	startTimes.pop_back();

	float time = endTime - startTime;

	TimeMap::iterator it = times.find(path);
	if (it != times.end()) {
		it->second += time;
	} else {
		times.insert(std::make_pair(path, time));
	}

	path = paths.size() > 0 ? paths.back() : "";

	//if (paths.size() == 0) {
	//	for (TimeMap::iterator it = times.begin(); it != times.end(); ++it) {
	//		LOGI("p: %s t: %f\n", it->first.c_str(), it->second);
	//	}
	//}
}

/*static*/
void PerfTimer::popPush( const std::string& name )
{
	pop();
	push(name);
}

/*static*/
std::vector<PerfTimer::ResultField> PerfTimer::getLog(const std::string& rawPath) {
	if (!enabled) return std::vector<ResultField>();

	std::string path = rawPath;

	TimeMap::const_iterator itRoot = times.find("root");
	float globalTime = (itRoot != times.end())? itRoot->second : 0;

	TimeMap::const_iterator itPath = times.find(path);
	float totalTime2 = (itRoot != times.end())? itRoot->second : -1;

	std::vector<ResultField> result;

	if (path.length() > 0) path += ".";
	float totalTime = 0;

	for (TimeMap::const_iterator cit = times.begin(); cit != times.end(); ++cit) {
		const std::string& key = cit->first;
		const float& time = cit->second;
		if (key.length() > path.length() && Util::startsWith(key, path) && key.find(".", path.length() + 1) == std::string::npos) {
			totalTime += time;
		}
	}

	float oldTime = totalTime;
	if (totalTime < totalTime2) totalTime = totalTime2;
	if (globalTime < totalTime) globalTime = totalTime;

	for (TimeMap::const_iterator cit = times.begin(); cit != times.end(); ++cit) {
		const std::string& key = cit->first;
		//const float& time = cit->second;
		if (key.length() > path.length() && Util::startsWith(key, path) && key.find(".", path.length() + 1) == std::string::npos) {
			float time = times.find(key)->second;
			float timePercentage = time * 100.0f / totalTime;
			float globalPercentage = time * 100.0f / globalTime;
			std::string name = key.substr(path.length());
			result.push_back(ResultField(name, timePercentage, globalPercentage));
		}
	}

	for (TimeMap::iterator it = times.begin(); it != times.end(); ++it)
		it->second *= 0.999f;

	if (totalTime > oldTime)
		result.push_back(ResultField("unspecified", (totalTime - oldTime) * 100.0f / totalTime, (totalTime - oldTime) * 100.0f / globalTime));

	std::sort(result.begin(), result.end());
	result.insert(result.begin(), ResultField(rawPath, 100, totalTime * 100.0f / globalTime));
	return result;
}
