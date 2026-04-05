#ifndef NET_MINECRAFT_CLIENT_PLAYER_INPUT_TOUCHSCREEN_TouchTurnInput_H__
#define NET_MINECRAFT_CLIENT_PLAYER_INPUT_TOUCHSCREEN_TouchTurnInput_H__

#include "../ITurnInput.h"
#include "TouchAreaModel.h"
#include "../../../../platform/input/Multitouch.h"

class TouchTurnInput: public ITurnInput
{
public:
	static const int MODE_OFFSET = 1;
	static const int MODE_DELTA  = 2;

	TouchTurnInput(int mode_)
	:	mode(mode_),
		cxO(0), cyO(0),
		wasActive(false)
	{
	}

	TurnDelta getTurnDelta() {
		float dx = 0, dy = 0;

		float cx = 0;
		float cy = 0;
		bool isActive = false;

		const int* pointerIds;
		int pointerCount = Multitouch::getActivePointerIds(&pointerIds);
		for (int i = 0; i < pointerCount; ++i) {
			int p = pointerIds[i];
			int x = Multitouch::getX(p);
			int y = Multitouch::getY(p);
			int areaId = _model.getPointerId(x, y, p);

			if (areaId == AREA_TURN) {
				isActive = true;
				cx = (float)x * 0.5f;
				cy = (float)y * -0.5f;
				break;
			}
		}

		if (MODE_OFFSET == mode) {
			//float dt = getDeltaTime();
			//const float MaxTurnX = 250.0f;
			//const float MaxTurnY = 200.0f;
			//float cx = isActive? Controller::getX(cid) : cxO * 0.7f;
			//float cy = isActive? Controller::getY(cid) : cyO * 0.7f;
			//dx = linearTransform( cx, 0.1f, MaxTurnX ) * dt;
			//dy = linearTransform( cy, 0.1f, MaxTurnY ) * dt;
			//cxO = cx;
			//cyO = cy;
		} else
			if (MODE_DELTA == mode && (wasActive || isActive)) {
//				const float dt = getDeltaTime();
//				const float MaxTurnX = 100.0f;
//				const float MaxTurnY = 100.0f;
				const float DeadZone = 0;//0.25f * dt;//0.02f;

				if (!wasActive) {
					cxO = cx;
					cyO = cy;
				}
				if (isActive) {
					dx = linearTransform(cx - cxO, DeadZone);// * MaxTurnX;
					dy = linearTransform(cy - cyO, DeadZone);// * MaxTurnY;
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

	TouchAreaModel _model;

	static const int AREA_TURN = 100;
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER_INPUT_TOUCHSCREEN_TouchTurnInput_H__*/
