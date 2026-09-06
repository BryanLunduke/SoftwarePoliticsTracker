#ifndef NET_MINECRAFT_CLIENT_PLAYER_INPUT_ControllerTurnInput_H__
#define NET_MINECRAFT_CLIENT_PLAYER_INPUT_ControllerTurnInput_H__

#include "ITurnInput.h"
#include "../../../platform/input/Controller.h"

/** A Controller Turn input */
class ControllerTurnInput : public ITurnInput {
public:
	static const int MODE_OFFSET = 1;
	static const int MODE_DELTA  = 2;

	ControllerTurnInput(int controllerId, int mode_)
	:	cid(controllerId),
		mode(mode_),
		cxO(0), cyO(0),
		wasActive(false)
	{}

	bool isTouched() { return Controller::isTouched(cid); }

	TurnDelta getTurnDelta() {
		float dx = 0, dy = 0;
		bool isActive = Controller::isTouched(cid);

		if (MODE_OFFSET == mode) {
			float dt = getDeltaTime();
			const float MaxTurnX = 250.0f;
			const float MaxTurnY = 200.0f;
			float cx = isActive? Controller::getX(cid) : cxO * 0.7f;
			float cy = isActive? Controller::getY(cid) : cyO * 0.7f;
			dx = linearTransform( cx, 0.1f, MaxTurnX ) * dt;
			dy = linearTransform( cy, 0.1f, MaxTurnY ) * dt;
			cxO = cx;
			cyO = cy;
		} else
			if (MODE_DELTA == mode && (wasActive || isActive)) {
				float cx = Controller::getX(cid);
				float cy = Controller::getY(cid);

//				const float dt = getDeltaTime();
				const float MaxTurnX = 100.0f;
				const float MaxTurnY = 100.0f;
				const float DeadZone = 0;//0.25f * dt;//0.02f;

				if (!wasActive) {
					cxO = cx;
					cyO = cy;
				}
				if (isActive) {
					dx = linearTransform(cx - cxO, DeadZone) * MaxTurnX;
					dy = linearTransform(cy - cyO, DeadZone) * MaxTurnY;
					cxO = cx;
					cyO = cy;
				}
			}

			wasActive = isActive;
			return TurnDelta(dx, -dy);
	}

	int mode;
private:
	int cid;
	float cxO, cyO;
	bool wasActive;
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER_INPUT_ControllerTurnInput_H__*/
