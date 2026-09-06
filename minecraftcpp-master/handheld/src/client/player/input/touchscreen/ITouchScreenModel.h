#ifndef NET_MINECRAFT_CLIENT_PLAYER__ITouchScreenModel_H__
#define NET_MINECRAFT_CLIENT_PLAYER__ITouchScreenModel_H__

#include "../../../../platform/input/Mouse.h"

class ITouchScreenModel
{
public:
	virtual ~ITouchScreenModel() {}
	virtual int getPointerId(const MouseAction& m) { return m.pointerId; }
	virtual int getPointerId(int x, int y, int pid) { return pid; }
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER__ITouchScreenModel_H__*/
