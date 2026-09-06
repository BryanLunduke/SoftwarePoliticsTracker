#ifndef NET_MINECRAFT_CLIENT_PLAYER_INPUT__IBuildInput_H__
#define NET_MINECRAFT_CLIENT_PLAYER_INPUT__IBuildInput_H__

#include "../../IConfigListener.h"
class Player;

class BuildActionIntention {
public:
	BuildActionIntention()
	:	action(0)
	{}
	BuildActionIntention(int action)
	:	action(action)
	{}

	bool isFirstRemove() { return (action & BAI_FIRSTREMOVE) != 0; }
	bool isRemoveContinue() { return (action & BAI_REMOVE) != 0; }

	bool isBuild() { return (action & BAI_BUILD) != 0; }
	bool isRemove() { return isFirstRemove() || isRemoveContinue(); }

	bool isAttack() { return (action & BAI_ATTACK) != 0; }
	bool isInteract() { return (action & BAI_INTERACT) != 0; }

	int action;

	static const int BAI_BUILD = 1;
	static const int BAI_REMOVE = 2;
	static const int BAI_FIRSTREMOVE = 4;

	static const int BAI_ATTACK = 8;
	static const int BAI_INTERACT = 16;
};


class IBuildInput: public IConfigListener {
public:
	virtual ~IBuildInput(){}

	virtual void onConfigChanged(const Config& c) {}
	// @return true if user wants to do an action, false if not
	virtual bool tickBuild(Player*, BuildActionIntention* bai) { return false; }
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER_INPUT__IBuildInput_H__*/
