#ifndef NET_MINECRAFT_CLIENT_PLAYER_INPUT_TOUCHSCREEN_TouchscreenInput_H__
#define NET_MINECRAFT_CLIENT_PLAYER_INPUT_TOUCHSCREEN_TouchscreenInput_H__

//package net.minecraft.client.player;

#include "../IMoveInput.h"
#include "../../../gui/GuiComponent.h"
#include "TouchAreaModel.h"
#include "../../../renderer/RenderChunk.h"

class Options;
class Player;
class Minecraft;
class PolygonArea;

// @todo: extract a separate MoveInput (-> merge XperiaPlayInput)
class TouchscreenInput_TestFps:	public IMoveInput,
								public GuiComponent
{
public:
    static const int KEY_UP = 0;
    static const int KEY_DOWN = 1;
    static const int KEY_LEFT = 2;
    static const int KEY_RIGHT = 3;
    static const int KEY_JUMP = 4;
    static const int KEY_SNEAK = 5;
	static const int KEY_CRAFT = 6;
	static const int NumKeys = 7;

    TouchscreenInput_TestFps(Minecraft* mc, Options* options);
	~TouchscreenInput_TestFps();

	void onConfigChanged(const Config& c);

	void tick(Player* player);
	void render(float a);

	void setKey(int key, bool state);
    void releaseAllKeys();

	const RectangleArea& getRectangleArea();
    const RectangleArea& getPauseRectangleArea();

private:
	void clear();

	RectangleArea _boundingRectangle;

	bool _keys[NumKeys];
	Options* _options;

	bool _pressedJump;
	bool _forward;
	bool _northJump;
	bool _renderFlightImage;
	TouchAreaModel _model;
	Minecraft* _minecraft;

	RectangleArea* aLeft;
	RectangleArea* aRight;
	RectangleArea* aUp;
	RectangleArea* aDown;
	RectangleArea* aPause;
	//RectangleArea* aUpJump;
	RectangleArea* aJump;
	RectangleArea* aUpLeft;
	RectangleArea* aUpRight;
	bool _pauseIsDown;

	RenderChunk _render;
	bool _allowHeightChange;
	float _sneakTapTime;

	bool _buttons[8];
	bool isButtonDown(int areaId);
	void rebuild();
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER_INPUT_TOUCHSCREEN_TouchscreenInput_H__*/
