#include "TouchscreenInput.h"
#include "../../../Options.h"
#include "../../../../platform/input/Multitouch.h"
#include "../../../gui/Gui.h"
#include "../../../renderer/Tesselator.h"
#include "../../../../world/entity/player/Player.h"

#include "../../../Minecraft.h"
#include "../../../../platform/log.h"
#include "../../../renderer/Textures.h"
#include "../../../sound/SoundEngine.h"

static const int AREA_DPAD_FIRST = 100;
static const int AREA_DPAD_N = 100;
static const int AREA_DPAD_S = 101;
static const int AREA_DPAD_W = 102;
static const int AREA_DPAD_E = 103;
static const int AREA_DPAD_C = 104;
static const int AREA_PAUSE = 105;

static int cPressed = 0;
static int cReleased = 0;
static int cDiscreet = 0;
static int cPressedPause = 0;
static int cReleasedPause = 0;
//static const int AREA_DPAD_N_JUMP = 105;

//
// TouchscreenInput_TestFps
//

static void Copy(int n, float* x, float* y, float* dx, float* dy) {
	for (int i = 0; i < n; ++i) {
		dx[i] = x[i];
		dy[i] = y[i];
	}
}

static void Translate(int n, float* x, float* y, float xt, float yt) {
	for (int i = 0; i < n; ++i) {
		x[i] += xt;
		y[i] += yt;
	}
}

static void Scale(int n, float* x, float* y, float xt, float yt) {
	for (int i = 0; i < n; ++i) {
		x[i] *= xt;
		y[i] *= yt;
	}
}

static void Transformed(int n, float* x, float* y, float* dx, float* dy, float xt, float yt, float sx=1.0f, float sy=1.0f) {
	Copy(n, x, y, dx, dy);
	Scale(n, dx, dy, sx, sy);
	Translate(n, dx, dy, xt, yt);

	//for (int i = 0; i < n; ++i) {
	//	LOGI("%d. (%f, %f)\n", i, dx[i], dy[i]);
	//}
}

TouchscreenInput_TestFps::TouchscreenInput_TestFps( Minecraft* mc, Options* options )
:	_minecraft(mc),
	_options(options),
	_northJump(false),
	_forward(false),
	_boundingRectangle(0, 0, 1, 1),
	_pressedJump(false),
	_pauseIsDown(false),
	_sneakTapTime(-999),
	aLeft(0),
	aRight(0),
	aUp(0),
	aDown(0),
	aJump(0),
	aUpLeft(0),
	aUpRight(0),
	_allowHeightChange(false)
{
	releaseAllKeys();
	onConfigChanged( createConfig(mc) );

	Tesselator& t = Tesselator::instance;
	const int alpha = 128;
	t.color( 0xc0c0c0, alpha); cPressed  = t.getColor();
	t.color( 0xffffff, alpha); cReleased = t.getColor();
	t.color( 0xffffff, alpha / 4); cDiscreet = t.getColor();
    t.color( 0xc0c0c0, 80); cPressedPause=t.getColor();
    t.color( 0xffffff, 80); cReleasedPause=t.getColor();
}

TouchscreenInput_TestFps::~TouchscreenInput_TestFps() {
	clear();
}

void TouchscreenInput_TestFps::clear() {
	_model.clear();

	delete aUpLeft; aUpLeft = NULL; // @todo: SAFEDEL
	delete aUpRight; aUpRight = NULL;
}

bool TouchscreenInput_TestFps::isButtonDown(int areaId) {
	return _buttons[areaId - AREA_DPAD_FIRST];
}


void TouchscreenInput_TestFps::onConfigChanged(const Config& c) {
	clear();

	const float w = (float)c.width;
	const float h = (float)c.height;

	/*
	// Code for "Move when touching left side of the screen"
	float x0[] = {  0,  w * 0.3f,  w * 0.3f,     0 };
	float y0[] = {	0,	       0,      h-32,  h-32 };

	_model.addArea(AREA_MOVE, new RectangleArea(0, 0, w*0.3f, h-32));
	*/

	// Code for "D-pad with jump in center"
	float Bw = w * 0.11f;//0.08f;
	float Bh = Bw;//0.15f;
    
    // If too large (like playing on Tablet)
    PixelCalc& pc = _minecraft->pixelCalc;
    if (pc.pixelsToMillimeters(Bw) > 14) {
        Bw = Bh = pc.millimetersToPixels(14);
    }
	// temp data
	float xx;
	float yy;

	const float BaseY = -8 + h - 3.0f * Bh;
	const float BaseX = _options->isLeftHanded? -8 + w - 3 * Bw
											:	8 + 0;
	// Setup the bounding rectangle
	_boundingRectangle = RectangleArea(BaseX, BaseY, BaseX + 3 * Bw, BaseY + 3 * Bh);

	xx = BaseX + Bw; yy = BaseY;
	_model.addArea(AREA_DPAD_N, aUp = new RectangleArea(xx, yy, xx+Bw, yy+Bh));
	xx = BaseX;
	aUpLeft = new RectangleArea(xx, yy, xx+Bw, yy+Bh);
	xx = BaseX + 2 * Bw;
	aUpRight = new RectangleArea(xx, yy, xx+Bw, yy+Bh);

	xx = BaseX + Bw; yy = BaseY + Bh;
	_model.addArea(AREA_DPAD_C, aJump = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

	xx = BaseX + Bw; yy = BaseY + 2 * Bh;
	_model.addArea(AREA_DPAD_S, aDown = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

	xx = BaseX; yy = BaseY + Bh;
	_model.addArea(AREA_DPAD_W, aLeft = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

	xx = BaseX + 2 * Bw; yy = BaseY + Bh;
	_model.addArea(AREA_DPAD_E, aRight = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

#ifdef __APPLE__
    float maxPixels = _minecraft->pixelCalc.millimetersToPixels(10);
    float btnSize = Mth::Min(18 * Gui::GuiScale, maxPixels);
	_model.addArea(AREA_PAUSE, aPause = new RectangleArea(w - 4 - btnSize,
                                                          4,
                                                          w - 4,
                                                          4 + btnSize));
#endif /* __APPLE__ */

	//rebuild();
}

void TouchscreenInput_TestFps::setKey( int key, bool state )
{
	#ifdef WIN32
        //LOGI("key: %d, %d\n", key, state);

		int id = -1;
		if (key == _options->keyUp.key) id = KEY_UP;
		if (key == _options->keyDown.key) id = KEY_DOWN;
		if (key == _options->keyLeft.key) id = KEY_LEFT;
		if (key == _options->keyRight.key) id = KEY_RIGHT;
		if (key == _options->keyJump.key) id = KEY_JUMP;
		if (key == _options->keySneak.key) id = KEY_SNEAK;
		if (key == _options->keyCraft.key) id = KEY_CRAFT;
		if (id >= 0) {
			_keys[id] = state;
		}
	#endif
}

void TouchscreenInput_TestFps::releaseAllKeys()
{
	xa = 0;
	ya = 0;

	for (int i = 0; i<8; ++i)
		_buttons[i] = false;
#ifdef WIN32
	for (int i = 0; i<NumKeys; ++i)
		_keys[i] = false;
#endif
	_pressedJump = false;
	_allowHeightChange = false;
}

void TouchscreenInput_TestFps::tick( Player* player )
{
	xa = 0;
	ya = 0;
	jumping = false;

	//bool gotEvent = false;
	bool heldJump = false;
	bool tmpForward = false;
	bool tmpNorthJump = false;

	for (int i = 0; i < 6; ++i)
		_buttons[i] = false;

	const int* pointerIds;
	int pointerCount = Multitouch::getActivePointerIdsThisUpdate(&pointerIds);
	for (int i = 0; i < pointerCount; ++i) {
		int p = pointerIds[i];
		int x = Multitouch::getX(p);
		int y = Multitouch::getY(p);

		if (_boundingRectangle.isInside((float)x, (float)y) && _forward && !isChangingFlightHeight)
		{
			float angle = Mth::PI + Mth::atan2(y - _boundingRectangle.centerY(), x - _boundingRectangle.centerX());
			ya = Mth::sin(angle);
			xa = Mth::cos(angle);
			tmpForward = true;
		}

		int areaId = _model.getPointerId(x, y, p);
		if (areaId < AREA_DPAD_FIRST)
		{
			continue;
		}

		bool setButton = false;

		if (Multitouch::isPressed(p))
			_allowHeightChange = (areaId == AREA_DPAD_C);

        if (areaId == AREA_DPAD_C)
		{
			setButton = true;
			heldJump = true;
			// If we're in water or pressed down on the button: jump
			if (player->isInWater()) {
				jumping = true;
			}
			else if (Multitouch::isPressed(p)) {
				jumping = true;
			} // Or if we are walking forward, jump while going forward!
			else if (_forward && !player->abilities.flying) {
				areaId = AREA_DPAD_N;
				tmpNorthJump = true;
				//jumping = true;
				ya += 1;
			}
		}

		if	(areaId == AREA_DPAD_N)
		{
			setButton = true;
			if (player->isInWater())
				jumping = true;
			else if (!isChangingFlightHeight)
				tmpForward = true;
			ya += 1;
		}
		else if (areaId == AREA_DPAD_S && !_forward)
		{
			setButton = true;
            ya -= 1;
			/*
            if (Multitouch::isReleased(p)) {
                float now = getTimeS();
                if (now - _sneakTapTime < 0.4f) {
                    ya += 1;
                    sneaking = !sneaking;
                    player->setSneaking(sneaking);
                    _sneakTapTime = -1;
                } else {
                    _sneakTapTime = now;
                }
            }
			*/
        }
		else if (areaId == AREA_DPAD_W && !_forward)
		{
			setButton = true;
			xa += 1;
		}
		else if (areaId == AREA_DPAD_E && !_forward)
		{
			setButton = true;
			xa -= 1;
		}
#ifdef __APPLE__
		else if (areaId == AREA_PAUSE) {
			if (Multitouch::isReleased(p)) {
                _minecraft->soundEngine->playUI("random.click", 1, 1);
				_minecraft->screenChooser.setScreen(SCREEN_PAUSE);
            }
		}
#endif /*__APPLE__*/
		_buttons[areaId - AREA_DPAD_FIRST] = setButton;
	}

	_forward = tmpForward;

	// Only jump once at a time
	if (tmpNorthJump) {
		if (!_northJump)
			jumping = true;
		_northJump = true;
	}
	else _northJump = false;

	isChangingFlightHeight = false;
	wantUp   = isButtonDown(AREA_DPAD_N) && (_allowHeightChange & (_pressedJump | wantUp));
	wantDown = isButtonDown(AREA_DPAD_S) && (_allowHeightChange & (_pressedJump | wantDown));
	if (player->abilities.flying && (wantUp || wantDown || (heldJump && !_forward)))
	{
		isChangingFlightHeight = true;
		ya = 0;
	}
	_renderFlightImage = player->abilities.flying;

#ifdef WIN32
	if (_keys[KEY_UP]) ya++;
	if (_keys[KEY_DOWN]) ya--;
	if (_keys[KEY_LEFT]) xa++;
	if (_keys[KEY_RIGHT]) xa--;
	if (_keys[KEY_JUMP]) jumping = true;
	//sneaking = _keys[KEY_SNEAK];
	if (_keys[KEY_CRAFT])
		player->startCrafting((int)player->x, (int)player->y, (int)player->z, Recipe::SIZE_2X2);
#endif

	if (sneaking) {
		xa *= 0.3f;
		ya *= 0.3f;
	}
	//printf("\n>- %f %f\n", xa, ya);
	_pressedJump = heldJump;
}

static void drawRectangleArea(Tesselator& t, RectangleArea* a, int ux, int vy, float ssz = 64.0f) {
	const float pm = 1.0f / 256.0f;
	const float sz = ssz * pm;
	const float uu = (float)(ux) * pm;
	const float vv = (float)(vy) * pm;
	const float x0 = a->_x0 * Gui::InvGuiScale;
	const float x1 = a->_x1 * Gui::InvGuiScale;
	const float y0 = a->_y0 * Gui::InvGuiScale;
	const float y1 = a->_y1 * Gui::InvGuiScale;

	t.vertexUV(x0, y1, 0, uu,	vv+sz);
	t.vertexUV(x1, y1, 0, uu+sz,vv+sz);
	t.vertexUV(x1, y0, 0, uu+sz,vv);
	t.vertexUV(x0, y0, 0, uu,	vv);
}

static void drawPolygonArea(Tesselator& t, PolygonArea* a, int x, int y) {
	float pm = 1.0f / 256.0f;
	float sz = 64.0f * pm;
	float uu = (float)(x) * pm;
	float vv = (float)(y) * pm;

	float uvs[] = {uu, vv, uu+sz, vv, uu+sz, vv+sz, uu, vv+sz};
	const int o = 0;

	for (int j = 0; j < a->_numPoints; ++j) {
		t.vertexUV(a->_x[j] * Gui::InvGuiScale, a->_y[j] * Gui::InvGuiScale, 0, uvs[(o+j+j)&7], uvs[(o+j+j+1)&7]);
	}
}

void TouchscreenInput_TestFps::render( float a ) {
	//return;

	//static Stopwatch sw;
	//sw.start();


	//glColor4f2(1, 0, 1, 1.0f);
	//glDisable2(GL_CULL_FACE);
	glDisable2(GL_ALPHA_TEST);

	glEnable2(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	_minecraft->textures->loadAndBindTexture("gui/gui.png");
	
	//glDisable2(GL_TEXTURE_2D);

	rebuild();
	//drawArrayVTC(_bufferId, 5 * 2 * 3, 24);

	glDisable2(GL_BLEND);
	//glEnable2(GL_TEXTURE_2D);
	//glEnable2(GL_CULL_FACE);

	//sw.stop();
	//sw.printEvery(100, "buttons");
}

const RectangleArea& TouchscreenInput_TestFps::getRectangleArea()
{
	return _boundingRectangle;
}
const RectangleArea& TouchscreenInput_TestFps::getPauseRectangleArea()
{
    return *aPause;
}

void TouchscreenInput_TestFps::rebuild() {
    if (_options->hideGui)
        return;
    
	Tesselator& t = Tesselator::instance;
	//LOGI("instance is: %p, %p, %p, %p, %p FOR %d\n", &t, aLeft, aRight, aUp, aDown, aJump, _bufferId);
	//t.setAccessMode(Tesselator::ACCESS_DYNAMIC);
	t.begin();

	const int imageU = 0;
	const int imageV = 107;
	const int imageSize = 26;

	bool northDiagonals = !isChangingFlightHeight && (_northJump || _forward);

	// render left button
	if (northDiagonals || isChangingFlightHeight) t.colorABGR(cDiscreet);
    else if (isButtonDown(AREA_DPAD_W)) t.colorABGR(cPressed);
	else						   t.colorABGR(cReleased);
	drawRectangleArea(t, aLeft, imageU + imageSize, imageV, (float)imageSize);

	// render right button
	if (northDiagonals || isChangingFlightHeight) t.colorABGR(cDiscreet);
	else if (isButtonDown(AREA_DPAD_E)) t.colorABGR(cPressed);
	else						   t.colorABGR(cReleased);
	drawRectangleArea(t, aRight, imageU + imageSize * 3, imageV, (float)imageSize);

	// render forward button
	if (isButtonDown(AREA_DPAD_N)) t.colorABGR(cPressed);
	else						   t.colorABGR(cReleased);
	if (isChangingFlightHeight)
	{
		drawRectangleArea(t, aUp, imageU + imageSize * 2, imageV + imageSize, (float)imageSize);
	}
	else
	{
		drawRectangleArea(t, aUp, imageU, imageV, (float)imageSize);
	}
	
	// render diagonals, if available
	if (northDiagonals)
	{
		t.colorABGR(cReleased);
		drawRectangleArea(t, aUpLeft, imageU, imageV + imageSize, (float)imageSize);
		drawRectangleArea(t, aUpRight, imageU + imageSize, imageV + imageSize, (float)imageSize);
	}

	// render backwards button
	if (northDiagonals) t.colorABGR(cDiscreet);
	else if (isButtonDown(AREA_DPAD_S)) t.colorABGR(cPressed);
	else						   t.colorABGR(cReleased);
	if (isChangingFlightHeight)
	{
		drawRectangleArea(t, aDown, imageU + imageSize * 3, imageV + imageSize, (float)imageSize);
	}
	else
	{
		drawRectangleArea(t, aDown, imageU + imageSize * 2, imageV, (float)imageSize);
	}

	// render jump / flight button
	if (_renderFlightImage && northDiagonals) t.colorABGR(cDiscreet);
	else if (isButtonDown(AREA_DPAD_C)) t.colorABGR(cPressed);
	else						   t.colorABGR(cReleased);
	if (_renderFlightImage)
	{
		drawRectangleArea(t, aJump, imageU + imageSize * 4, imageV + imageSize, (float)imageSize);
	}
	else
	{
		drawRectangleArea(t, aJump, imageU + imageSize * 4, imageV, (float)imageSize);
	}
	

#ifdef __APPLE__
	if (!_minecraft->screen) {
		if (isButtonDown(AREA_PAUSE))  t.colorABGR(cPressedPause);
		else						   t.colorABGR(cReleasedPause);
		
        drawRectangleArea(t, aPause, 200, 64, 18.0f);
	}
#endif /*__APPLE__*/
//t.end(true, _bufferId);
	//return;

	t.draw();
	//RenderChunk _render = t.end(true, _bufferId);
	//t.setAccessMode(Tesselator::ACCESS_STATIC);
	//_bufferId = _render.vboId;
}
