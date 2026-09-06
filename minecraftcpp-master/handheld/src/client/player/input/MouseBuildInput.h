#ifndef NET_MINECRAFT_CLIENT_PLAYER_INPUT_MouseBuildInput_H__
#define NET_MINECRAFT_CLIENT_PLAYER_INPUT_MouseBuildInput_H__

#include "IBuildInput.h"
#include "../../../platform/input/Mouse.h"

/** A Mouse Build input */
class MouseBuildInput : public IBuildInput {
public:
	MouseBuildInput()
	:	buildDelayTicks(10),
		buildHoldTicks(0)
	{}

	virtual bool tickBuild(Player* p, BuildActionIntention* bai) {
		if (Mouse::getButtonState(MouseAction::ACTION_LEFT) != 0) {
			*bai = BuildActionIntention(BuildActionIntention::BAI_REMOVE | BuildActionIntention::BAI_ATTACK);
			return true;
		}
		if (Mouse::getButtonState(MouseAction::ACTION_RIGHT) != 0) {
			if (buildHoldTicks >= buildDelayTicks) buildHoldTicks = 0;
			if (++buildHoldTicks == 1) {
				*bai = BuildActionIntention(BuildActionIntention::BAI_BUILD | BuildActionIntention::BAI_INTERACT);
				return true;
			}
		} else {
			buildHoldTicks = 0;
		}
		return false;
	}
private:
	int buildHoldTicks;
	int buildDelayTicks;
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER_INPUT_MouseBuildInput_H__*/
