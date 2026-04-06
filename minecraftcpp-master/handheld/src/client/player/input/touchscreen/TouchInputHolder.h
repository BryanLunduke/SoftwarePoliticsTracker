#ifndef NET_MINECRAFT_CLIENT_PLAYER_INPUT_TOUCHSCREEN_TouchInputHolder_H__
#define NET_MINECRAFT_CLIENT_PLAYER_INPUT_TOUCHSCREEN_TouchInputHolder_H__

#include "../IInputHolder.h"
#include "TouchscreenInput.h"

#include "../ITurnInput.h"
#include "TouchAreaModel.h"
#include "../../../../platform/input/Multitouch.h"
#include "../../../../platform/time.h"
#include "../../../../util/SmoothFloat.h"

#include "../../../../world/entity/player/Player.h"
#include "../../../../world/entity/player/Inventory.h"

template <class T>
class ModifyNotify {
public:
	ModifyNotify()
	:	_changed(false)
	{}

	T& getOld() { return _old; }
	T& getNew() { return _new; }

	bool update(T& val) {
		_changed = !equals(val);
		if (_changed) {
			_old = _new;
			_new = val;
		}
		return _changed;
	}

	bool hasChanged() const { return _changed; }

	virtual bool equals(const T& newVal) {
		return _old != newVal;
	}
protected:
	T _old;
private:
	T _new;
	bool _changed;
};

//
// Implementation for unified [Turn & Build Input]
//
class UnifiedTurnBuild: public GuiComponent,
						public ITurnInput,
						public IBuildInput
{
public:
	static const int MODE_OFFSET = 1;
	static const int MODE_DELTA  = 2;

	UnifiedTurnBuild(int turnMode, int width, int height, float maxMovementDelta, float sensitivity, IInputHolder* holder, Minecraft* minecraft)
	:	mode(turnMode),
		_holder(holder),
		_options(&minecraft->options),
		cxO(0), cyO(0),
		wasActive(false),
		_totalMoveDelta(0),
		_maxMovement(maxMovementDelta),
		_lastPlayer(0),
		screenArea(-1, -1, 0, 0),
		allowPicking(false),
		state(State_None),
		moveArea(-1,-1,0,0),
		joyTouchArea(-1, -1, 0, 0),
		inventoryArea(-1,-1, 0, 0),
		pauseArea(-1, -1, 0, 0),
		_buildMovement(0),
		_sentFirstRemove(false),
		_canDestroy(false),
		_forceCanUse(false)
	{
		_area.deleteMe = false;
		setSensitivity(sensitivity);
		//((ITurnInput*)this)->onConfigChanged(createConfig(minecraft));
		onConfigChanged(createConfig(minecraft));

		_lastBuildDownTime = _startTurnTime = getTimeS();
	}

	void setSensitivity(float sensitivity) {
		_sensitivity = sensitivity;
	}

	virtual void onConfigChanged(const Config& c) {
		if (false && _options->isJoyTouchArea) {
			int touchWidth = c.width - (int)inventoryArea._x1;
			if (touchWidth > (int)c.minecraft->pixelCalc.millimetersToPixels(60))
				touchWidth = (int)c.minecraft->pixelCalc.millimetersToPixels(60);

			int touchHeight = (int)(c.height * 0.4f);
			if (touchHeight > (int)c.minecraft->pixelCalc.millimetersToPixels(40))
				touchHeight = (int)c.minecraft->pixelCalc.millimetersToPixels(40);

			joyTouchArea._x0 = (float)(c.width  - touchWidth);
			joyTouchArea._y0 = (float)(c.height - touchHeight);
			joyTouchArea._x1 = (float)c.width;
			joyTouchArea._y1 = (float)c.height;

			_area.clear();
			_area.include (&joyTouchArea);
			_model.clear();
			_model.addArea(AREA_TURN, &_area);
		} else {
			screenArea = RectangleArea(0, 0, (float)c.width, (float)c.height);
			// Expand the move area a bit
            const float border = 10;
			const float widen = (moveArea._x1-moveArea._x0) * 0.05f + border; // ~5% wider
			moveArea._x0 -= widen;
			moveArea._x1 += widen;
			const float heighten = (moveArea._y1-moveArea._y0) * 0.05f + border; // ~5% taller
			moveArea._y0 -= heighten;
			moveArea._y1 += heighten;
            
            pauseArea._x0 -= border;
            pauseArea._x1 += border;
            pauseArea._y0 -= border;
            pauseArea._y1 += border;

			//LOGI("move: %f, %f, %f, %f\n", moveArea._x0, moveArea._y0, moveArea._x1, moveArea._y1);

			_area.clear();
			_area.include(&screenArea);
			_area.exclude(&moveArea);
			_area.exclude(&inventoryArea);
#ifdef __APPLE__
            _area.exclude(&pauseArea);
#endif /*__APPLE__*/
			//LOGI("Movearea: %f %f %f% f\n", moveArea._x0, moveArea._x1, moveArea._y0, moveArea._y1);

			_model.clear();
			_model.addArea(AREA_TURN, &_area);
		}
	}

	float calcNewAlpha(float current, float wanted) {
		if (wanted > current)
			return Mth::clamp(current + 0.02f, 0.0f, wanted);
		if (wanted < current)
			return Mth::clamp(current - 0.04f, wanted, 1.0f);
		return current;
	}

	//
	// Implementation for the ITurnInput part
	//
	TurnDelta getTurnDelta() {
		float dx = 0, dy = 0;
		const float now = getTimeS();

		float cx = 0;
		float cy = 0;
		bool isActive = false;

		const int* pointerIds;
		bool wasFirstMovement = false;
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
				wasFirstMovement = Multitouch::wasFirstMovement(p);
				break;
			}
		}
		if (isActive && !wasActive) {
			_startTurnTime = now;
			_totalMoveDelta = 0;
			bool isInMovement = _lastPlayer? getSpeedSquared(_lastPlayer) > 0.01f/*.25f*/ : false;
			//state = isInMovement? State_Turn : State_Deciding;
			state = State_Deciding;
			_canDestroy = !isInMovement;
			_forceCanUse = false;
			if (!_canDestroy && (_lastPlayer && _lastPlayer->canUseCarriedItemWhileMoving())) {
				_forceCanUse = true;
				_canDestroy = true;
			}
			_sentFirstRemove = false;
		} else if (wasActive && !isActive) {
			_sentFirstRemove = false;
			state = State_None;
			//_inBuild = false;
		}

		if (MODE_DELTA == mode && (wasActive || isActive)) {
//			const float dt = getDeltaTime();
//			const float MaxTurnX = 100.0f;
//			const float MaxTurnY = 100.0f;
			const float DeadZone = 0;//0.25f * dt;//0.02f;

			if (!wasActive) {
				cxO = cx;
				cyO = cy;
			}
			if (isActive) {
				dx = _sensitivity * linearTransform(cx - cxO, DeadZone);// * MaxTurnX;
				dy = _sensitivity * linearTransform(cy - cyO, DeadZone);// * MaxTurnY;
				
				float moveDelta = ( Mth::abs(dx) + Mth::abs(dy) );
				//LOGI("moveDelta is : %f\n", moveDelta);

				if (moveDelta > _maxMovement) {
					dx = 0;
					dy = 0;
					moveDelta = 0;
				}
				_totalMoveDelta += moveDelta;

				if (state == State_Deciding && _totalMoveDelta >= MaxBuildMovement/* && !_forceCanUse*/)
					state = State_Turn;

				// If a certain time has passed since we pressed button; check if
				// we've moved enough to prevent removal state to become activated
				const float since = now - _startTurnTime;
				if (state == State_Deciding && (since >= (0.001f*RemovalMilliseconds))) {
					//LOGI("DECIDED!: %f\n", _totalMoveDelta);
					bool isInMovement = _lastPlayer? getSpeedSquared(_lastPlayer) > 0.01f/*.25f*/ : false;
					if (!_forceCanUse && (_totalMoveDelta > 20.0f || isInMovement)) {
						state = State_Turn;
					} else {
						state = State_Destroy;
						_canDestroy = true;
					}
				}

				// Removing the pointer ketchup-effect that's built into some devices
				if (wasFirstMovement) {
					dx = dy = 0;
				}

				cxO = cx;
				cyO = cy;
                //LOGI("move delta: %f (%f, %f), %d\n", moveDelta, dx, dy, state);
			}
		} else {
            // Wasn't active, and isn't active!
            state = State_None;
        }

		updateFeedbackProgressAlpha(now);

		wasActive = isActive;
		return TurnDelta(dx, -dy);
	}

	void updateFeedbackProgressAlpha(float now) {
		const float MinAlphaValue = -0.05f;
		if (_canDestroy) {
			// Hack to test out fading in feedback circle
			const float since = now - _startTurnTime;
			if (state == State_Deciding) {
				const float wantedAlpha = since / (0.001f*RemovalMilliseconds);
				_holder->alpha = wantedAlpha * wantedAlpha;
			} else {
				if (state == State_Destroy) {
					_holder->alpha = calcNewAlpha(_holder->alpha, 1);
				} else if (state == State_Turn || state == State_None) {
					_holder->alpha = calcNewAlpha(_holder->alpha, 0);
				}

			}
		} else {
			_holder->alpha = MinAlphaValue;
		}
		//LOGI("state: %d, canDestroy: %d %d\n", state, _canDestroy, _forceCanUse);
		//LOGI("alpha: %f\n", _holder->alpha);
	}

	bool isInsideArea(float x, float y) {
		return _area.isInside(x, y);
	}

	int mode;

	static float getSpeedSquared(Entity* m) {
		const float xd = m->x - m->xo;
		const float yd = m->y - m->yo;
		const float zd = m->z - m->zo;
		const float speedSquared = xd*xd + yd*yd + zd*zd;
		return speedSquared;
	}

	void render(float alpha) {
		if (_options->isJoyTouchArea) {
			fill(	(int) (Gui::InvGuiScale * joyTouchArea._x0),
					(int) (Gui::InvGuiScale * joyTouchArea._y0),
					(int) (Gui::InvGuiScale * joyTouchArea._x1),
					(int) (Gui::InvGuiScale * joyTouchArea._y1), 0x40000000);//0x20ffffff);
		}
	}

	//
	// Implementation for the IBuildInput part
	//
	virtual bool tickBuild(Player* player, BuildActionIntention* bai) {
		_lastPlayer = player;

		if (state == State_Destroy) {
			if (!_sentFirstRemove) {
				*bai = BuildActionIntention((_forceCanUse?0:BuildActionIntention::BAI_FIRSTREMOVE) | BuildActionIntention::BAI_INTERACT);
				_sentFirstRemove = true;
			} else {
				*bai = BuildActionIntention((_forceCanUse?0:BuildActionIntention::BAI_REMOVE) | BuildActionIntention::BAI_INTERACT);
			}
			return true;
		}

		// Rewind the queue, in case someone has used it up before
		Multitouch::rewind();
		const float now = getTimeS();
		allowPicking = false;
		bool handled = false;

		while (Multitouch::next()) {
			MouseAction& m = Multitouch::getEvent();
			if (m.action == MouseAction::ACTION_MOVE) continue;

			int areaId = _model.getPointerId(m.x, m.y, m.pointerId);
			if (areaId != AREA_TURN) continue;

			allowPicking = true;
			//LOGI("down? %d, up? %d (%d, %d)\n", z && (m.data == MouseAction::DATA_DOWN), z && (m.data == MouseAction::DATA_UP), m.x, m.y); 

			if (_totalMoveDelta <= MaxBuildMovement && (m.data == MouseAction::DATA_UP && !handled)) {
				const float since = now - _lastBuildDownTime;
				//LOGI("move: (%d) %.2f - %f\n", state, _totalMoveDelta, since);
				if (state <= State_Deciding) {
					if (since >= 0.0f && since < 0.25f) {
						// We've pressed and released in haste; Let's build!
						*bai = BuildActionIntention(BuildActionIntention::BAI_BUILD | BuildActionIntention::BAI_ATTACK);
						handled = true;
					}
				}
				state = State_None;
			} else if (m.data == MouseAction::DATA_DOWN) {
				_lastBuildDownTime = now;
				_buildMovement = 0;
				state = State_Deciding;
			}
		}
		return handled;
	}

	bool allowPicking;
	float alpha;
	SmoothFloat smoothAlpha;
	
	RectangleArea screenArea;
	RectangleArea moveArea;
	RectangleArea joyTouchArea;
	RectangleArea inventoryArea;
    RectangleArea pauseArea;

private:
	IInputHolder* _holder;

	// Turn
	int cid;
	float cxO, cyO;
	bool wasActive;

	TouchAreaModel _model;
	IncludeExcludeArea _area;

	bool _decidedTurnMode;

	float _startTurnTime;
	float _totalMoveDelta;
	float _maxMovement;
	float _sensitivity;

	Player* _lastPlayer;

	// Build
	float _lastBuildDownTime;
	float _buildMovement;
	bool _sentFirstRemove;
	bool _canDestroy;
	bool _forceCanUse;

	int state;
	static const int State_None = 0;
	static const int State_Deciding = 1;
	static const int State_Turn = 2;
	static const int State_Destroy = 3;
	static const int State_Build = 4; // Will never happen

	static const int MaxBuildMovement = 20;
	static const int RemovalMilliseconds = 400;

	static const int AREA_TURN = 100;
	Options* _options;
};

class Minecraft;

#if defined(_MSC_VER)
	#pragma warning( disable : 4355 ) // 'this' pointer in initialization list which is perfectly legal
#endif

class TouchInputHolder: public IInputHolder
{
public:
	TouchInputHolder(Minecraft* mc, Options* options)
	:	_mc(mc),
		_move(mc, options),
		_turnBuild(UnifiedTurnBuild::MODE_DELTA, mc->width, mc->height, (float)MovementLimit, 1, this, mc)
	{
		onConfigChanged(createConfig(mc));
	}
	~TouchInputHolder() {
	}

	virtual void onConfigChanged(const Config& c) {
		_move.onConfigChanged(c);
		_turnBuild.moveArea = _move.getRectangleArea();
#ifdef __APPLE__
		_turnBuild.pauseArea = _move.getPauseRectangleArea();
#endif
		_turnBuild.inventoryArea = _mc->gui.getRectangleArea( _mc->options.isLeftHanded? 1 : -1 );
		_turnBuild.setSensitivity(c.options->isJoyTouchArea? 1.8f : 1.0f);
		((ITurnInput*)&_turnBuild)->onConfigChanged(c);
	}

	virtual bool allowPicking() {
		const int* pointerIds;
		int pointerCount = Multitouch::getActivePointerIds(&pointerIds);
		for (int i = 0; i < pointerCount; ++i) {
			int p = pointerIds[i];
			const float x = Multitouch::getX(p);
			const float y = Multitouch::getY(p);

			if (_turnBuild.isInsideArea(x, y)) {
				mousex = x;
				mousey = y;
				return true;
			}
		}

		return false;
		// return _turnBuild.allowPicking;
	}

	virtual void render(float alpha) {
		_turnBuild.render(alpha);
	}

	virtual IMoveInput*		getMoveInput()  { return &_move; }
	virtual ITurnInput*		getTurnInput()  { return &_turnBuild; }
	virtual IBuildInput*	getBuildInput() { return &_turnBuild; }

private:
	TouchscreenInput_TestFps _move;
	UnifiedTurnBuild _turnBuild;

	Minecraft* _mc;

	static const int MovementLimit = 200; // per update
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER_INPUT_TOUCHSCREEN_TouchInputHolder_H__*/
