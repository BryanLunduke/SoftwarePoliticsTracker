#include "ScrollingPane.h"
#include "../../renderer/gles.h"
#include "../Gui.h"
#include "../../../util/Mth.h"
#include "../../../SharedConstants.h"

#define STR(x) (x.toString().c_str())

static const float kPenetrationDeceleration = 0.03f;
static const float kPenetrationAcceleration = 0.08f;
static const float kMaxTrackingTime = 100.0f;
static const float kAcceleration = 15;
static const float kMinVelocityForDecelerationWithPaging = 4 / 3.0f;
static const float kMinVelocityForDeceleration = 1 / 3.0f;
static const float kDesiredAnimationFrameRate = 1000.0f / 60;
static const float kDecelerationFrictionFactor = 0.95f;
static const float kMinimumVelocityToHideScrollIndicators = 0.05f;
static const float kMinimumVelocity = 0.01f;
static const float kMinimumTrackingForDrag = 5;
static const float kMinIndicatorLength = 34.0f / 3;
static const float PKScrollIndicatorEndSize = 3;
static const float PKScrollIndicatorThickness = 7.0f /3;

ScrollingPane::ScrollingPane(
		int optionFlags,
		const IntRectangle& boundingBox,
		const IntRectangle& itemRect,
		int columns,
		int numItems,
		float screenScale /* = 1.0f */,
		const IntRectangle& itemBoundingRect /*= IntRectangle(0,0,0,0)*/ )
:	flags(optionFlags),
	bbox(boundingBox),
	size(boundingBox),
	area((float)bbox.x, (float)bbox.y, (float)(bbox.x + bbox.w), (float)(bbox.y + bbox.h)),
	bboxArea(area),
	itemRect(itemRect),
	numItems(numItems),
	screenScale(screenScale),
	invScreenScale(1.0f / screenScale),
	//hasItemBounding(),
	px(0), py(0), fpx(0), fpy(0),
	dx(0), dy(0),
	dragState(-1),
	dragLastDeltaTimeStamp(-1.0f),
	friction(0.95f),
	highlightTimer(-1),
	highlightStarted(-1),
	selectedId(-1),
	decelerating(false),
	tracking(false),
	dragging(false),
	pagingEnabled(false),
	_scrollEnabled(true),
	_timer(60),
	_doStepTimer(false),
	_wasDown(false),//Mouse::isButtonDown(MouseAction::ACTION_LEFT)),
	_lx(-9999), _ly(-9999)
{
	if (itemBoundingRect.w > 0)
		itemBbox = itemBoundingRect;
	else
		itemBbox = itemRect;

	this->columns = (columns>0)? columns : bbox.w / itemBbox.w;

	if (this->columns <= 0) {
		LOGW("Columns are 0! Area width is smaller than item width. Setting columns to 1.\n");
		this->columns = 1;
	}
	//LOGI("%d, %d :: %d\n", bbox.w, itemBbox.w, this->columns);

	rows = 1 + (numItems-1) / this->columns,

	/*
	if (columns * itemBbox.w <= bbox.w) flags |= SF_LockX;
	if (rows    * itemBbox.h <= bbox.h) flags |= SF_LockY;
	*/

	dragDeltas.reserve(128);

	te_moved = 0;
	te_ended = 1;

	selected = new bool[numItems];
	for (int i = 0; i < numItems; ++i)
		selected[i] = false;

	// Setup the scroll bars
	vScroll.w = vScroll.h = PKScrollIndicatorThickness;
	hScroll.w = hScroll.h = PKScrollIndicatorThickness;
	vScroll.x = bbox.x + bbox.w - vScroll.w;
	vScroll.y = 0;
	hScroll.x = 0;
	hScroll.y = bbox.y + bbox.h - hScroll.h;

//	vScroll.alpha
//	vScroll.fading = hScroll.fading = -1;
}

ScrollingPane::~ScrollingPane() {
	delete[] selected;
}

//void ScrollingPane::init(Minecraft* mc, int width, int height) {
//	this->mc = mc;
//	this->width = width;
//	this->height = height;
//}

void ScrollingPane::tick() {

	if (isSet(SF_ShowScrollbar)) {
		updateScrollFade(vScroll);
		updateScrollFade(hScroll);
	}
}

bool ScrollingPane::getGridItemFor_slow(int itemIndex, GridItem& out) {
	GridItem nw = getItemForPos(0, 0, false);
	GridItem se = getItemForPos((float)bbox.w - 1, (float)bbox.h - 1, false);
	const float bxx = bbox.x - (fpx /*+ dx*alpha*/) + (nw.xf - nw.x);
	const float byy = bbox.y - (fpy /*+ dy*alpha*/) + (nw.yf - nw.y);

	int y = itemIndex / columns;
	int x = itemIndex - (y * columns);

	out.id = itemIndex;
	out.xf  = bxx + x * itemBbox.w;
	out.yf  = byy + y * itemBbox.h;
	out.x = x;
	out.y = y;

	return	x >= nw.x && x <= se.x
		&&	y >= nw.y && y <= se.y;
}


void ScrollingPane::render( int xm, int ym, float alpha ) {
	// Handle user interaction first
	handleUserInput();

	_timer.advanceTime();

	if (_doStepTimer && !te_moved) {
		for (int i = 0; i < _timer.ticks; ++i)
			stepThroughDecelerationAnimation(false);
		this->lastFrame = getTimeMs();
	}

	// Render
	//if (isSet(SF_Scissor)) {
	//	glEnable2(GL_SCISSOR_TEST);
	//	GLuint x = (GLuint)(screenScale * bbox.x);
	//	GLuint y = 480 - (GLuint)(screenScale * (bbox.y + bbox.h));
	//	GLuint w = (GLuint)(screenScale * bbox.w);
	//	GLuint h = (GLuint)(screenScale * bbox.h);
	//	glScissor(x, y, w, h);
	//	LOGI("x, y, w, h: %d, %d, %d, %d\n", x, y, w, h);
	//}

	std::vector<GridItem> itemsToRender;
	GridItem nw = getItemForPos(0, 0, false);
	GridItem se = getItemForPos((float)bbox.w - 1, (float)bbox.h - 1, false);

	//LOGI("getItem: %d, %d - %d, %d\n", nw.x, nw.y, se.x, se.y);

	const float bxx = bbox.x - (fpx /*+ dx*alpha*/) + (nw.xf - nw.x);
	const float byy = bbox.y - (fpy /*+ dy*alpha*/) + (nw.yf - nw.y);
	for (int y = nw.y; y <= se.y; ++y)
	for (int x = nw.x; x <= se.x; ++x) {
		int id = y * columns + x;
		if (y <0 || id < 0 || id >= numItems) continue; // @todo: break rather
		if (isNotSet(SF_WrapX) && (x < 0 || x >= columns)) continue; // @todo: break rather
		GridItem item; //@todo: v- Does not support SF_Wrapping
		item.id = id;
		item.xf  = bxx + x * itemBbox.w;
		item.yf  = byy + y * itemBbox.h;
		item.x = (int)item.xf;
		item.y = (int)item.yf;
		//LOGI("i: %d (%.1f, %.1f)\t", id, item.xf, item.yf);
		if (isSet(SF_MultiSelect)) item.selected = selected[id];
		else item.selected = (id == selectedId);

		itemsToRender.push_back(item);
	}
	renderBatch(itemsToRender, alpha);

	//if (isSet(SF_Scissor))
	//	glDisable2(GL_SCISSOR_TEST);
}

void ScrollingPane::renderBatch(std::vector<GridItem>& items, float alpha) {
	for (unsigned int i = 0; i < items.size(); ++i)
		renderItem(items[i], alpha);
}

void ScrollingPane::renderItem(GridItem& item, float alpha) {
}

ScrollingPane::GridItem ScrollingPane::getItemForPos( float x, float y, bool isScreenPos ) {
	// Screen relative pos (rather than ScrollingPane relative pos)
	if (isScreenPos) {
		x -= bbox.x;
		y -= bbox.y;
	}

	// Add the scrolled offset
	x += fpx;
	y += fpy;

	// Does the grid SF_Wrap around?
	if (isSet(SF_WrapX)) x = fmod(x, (float)(itemBbox.w * columns));
	if (isSet(SF_WrapY)) y = fmod(y, (float)(itemBbox.h * rows));

	GridItem out;
	out.xf = x / itemBbox.w;
	out.yf = y / itemBbox.h;
	out.x  = (int) out.xf;
	out.y  = (int) out.yf;
	out.id = out.y * columns + out.x;
	return out;
}

void ScrollingPane::addDeltaPos(float x, float y, float dt, int a) {
	if (dt <= 0)
		return;

	Vec3 delta = (dragLastPos - Vec3(x, y, 0)) * (1.0f / dt);
	dragDeltas.push_back(delta.x);
	dragDeltas.push_back(delta.y);
	dragLastPos.set(x, y, 0);
	dragLastDeltaTimeStamp += dt; // @attn @fix: This relies on user to be correct
	//LOGI(">> delta %d: %s\n", a, STR(delta));
}

static const int PKTableViewMinTouchDurationForCellSelection = 150;

void ScrollingPane::handleUserInput() {

	bool isDown = Mouse::isButtonDown(MouseAction::ACTION_LEFT);
	float x = Mouse::getX() * invScreenScale;
	float y = Mouse::getY() * invScreenScale;
	int   t = getTimeMs();
	bool isInside = area.isInside(x, y);
	//LOGI("inside? %d\n", isInside);

	bool moved = (x != _lx || y != _ly);

	_lx = x;
	_ly = y;

	if (te_ended > 0 && _wasDown && !isDown)
		touchesEnded(x, y, t);
	else if (isDown && !_wasDown && isInside)
		touchesBegan(x, y, t);
	else if (te_moved > 0 && moved && isDown)
		touchesMoved(x, y, t);

	if (highlightTimer >= 0 && isNotSet(SF_NoHoldSelect)) {
		if (getTimeMs() - highlightTimer >= PKTableViewMinTouchDurationForCellSelection)
			onHoldItem();
	}

	_wasDown = isDown;
}

Vec3& ScrollingPane::contentOffset() {
	return _contentOffset;
}

void ScrollingPane::beginTracking(float x, float y, int t) { //@param 1: MouseEvent a
	if (this->tracking) {
		return;
	}
	//a.preventDefault();
	this->stopDecelerationAnimation();
	//this->hostingLayer.style.webkitTransitionDuration = 0;
	this->adjustContentSize(); //@todo @?
	this->minPoint.set((float)(this->size.w - this->adjustedContentSize.w), (float)(this->size.h - this->adjustedContentSize.h), 0); //@todo
	this->snapContentOffsetToBounds(false);
	this->startPosition = this->_contentOffset;
	this->startTouchPosition.set(x, y, 0);
	this->startTime = (float)t;
	this->startTimePosition = this->contentOffset();
	this->tracking = true;
	this->dragging = false;
	this->touchesHaveMoved = false;
	//window.addEventListener(PKMoveEvent, this, true); //@todo
	//window.addEventListener(PKEndEvent, this, true);
	//window.addEventListener("touchcancel", this, true);
	//window.addEventListener(PKEndEvent, this, false)
};

void ScrollingPane::touchesBegan(float x, float y, int t) { //@param 1: MouseEvent a
	if (!this->_scrollEnabled) {
		return;
	}

	te_ended = 1;
	//if (a.eventPhase == Event.CAPTURING_PHASE) {
	//	if (a._manufactured) {
	//		return
	//	}
			//this->highlightItem = getItemForPos(x, y, true);
	//	if (this.delaysContentTouches) {
	//		a.stopPropagation();
	//		this.callMethodNameAfterDelay("beginTouchesInContent", kContentTouchesDelay, a);
			this->beginTracking(x, y, t);
	//	}
	//} else {
	//	this.beginTracking(a)
	//}
	te_moved = 2;

	GridItem gi = getItemForPos(x, y, true);
	if (gi.id >= 0 && gi.id < numItems) {
		//LOGI("Pressed down at %d (%d, %d)\n", gi.id, gi.x, gi.y);
		highlightItem.id = bboxArea.isInside(x, y)? gi.id : -1;
		highlightStarted = highlightTimer = bboxArea.isInside(x, y)? getTimeMs() : -1;
	} else {
        highlightItem.id = -1;
        highlightStarted = highlightTimer = -1;
    }
}

void ScrollingPane::touchesMoved(float x, float y, int t)
{
	this->touchesHaveMoved = true;
	//this->callSuper(d);
	Vec3 e(x, y, 0);
	float b = e.x - this->startTouchPosition.x;
	float c = e.y - this->startTouchPosition.y;
	if (!this->dragging) {
		if ((Mth::abs(b) >= kMinimumTrackingForDrag && isNotSet(SF_LockX)) || (Mth::abs(c) >= kMinimumTrackingForDrag && isNotSet(SF_LockY))) {
			willBeginDragging();
			this->dragging = true;
			this->firstDrag = true;

			if (isSet(SF_ShowScrollbar)) {
				if (isNotSet(SF_LockX) && (this->adjustedContentSize.w > this->size.w))
					//this->hScroll.visible = true;
					this->hScroll.fading = 1;
				if (isNotSet(SF_LockY) && (this->adjustedContentSize.h > this->size.h))
					//this->vScroll.visible = true;
					this->vScroll.fading = 1;
			}
		}
	}
	if (this->dragging) {
		//d.stopPropagation();
		float f = isNotSet(SF_LockX) ? (this->startPosition.x + b) : this->_contentOffset.x;
		float a = isNotSet(SF_LockY) ? (this->startPosition.y + c) : this->_contentOffset.y;
		if (isNotSet(SF_HardLimits)) {
			f -= ((f < this->minPoint.x) ? (f - this->minPoint.x) : ((f > 0) ? f : 0)) / 2;
			a -= ((a < this->minPoint.y) ? (a - this->minPoint.y) : ((a > 0) ? a : 0)) / 2;
		} else {
			f = Mth::Min(Mth::Max(this->minPoint.x, f), 0.0f);
			a = Mth::Min(Mth::Max(this->minPoint.y, a), 0.0f);
		}
		if (this->firstDrag) {
			this->firstDrag = false;
			this->startTouchPosition = e;
			return;
		}
		this->setContentOffset(f, a);
		this->lastEventTime = t;//d.timeStamp;
	}
}

void ScrollingPane::touchesEnded(float x, float y, int t) {
	te_ended = 0;
	highlightStarted = -1;
	//te_moved = 0;

	//this.callSuper(a);
	this->tracking = false;
	if (this->dragging) {
		this->dragging = false;
		//a.stopPropagation();
		if (t - this->lastEventTime <= kMaxTrackingTime) {
			this->_contentOffsetBeforeDeceleration = this->_contentOffset;
			this->startDecelerationAnimation(false);
		}
		if (!this->decelerating) {}
		//window.removeEventListener(PKEndEvent, this, false);
		didEndDragging();
	}
	if (!this->decelerating) {
		if (fpy < 0 || fpy > bbox.h) { //@todo: for x as well (or rather, x^y)
			this->_contentOffsetBeforeDeceleration = this->_contentOffset;
			this->startDecelerationAnimation(true);
		} else {
			this->snapContentOffsetToBounds(true); //@fix
			this->hideScrollIndicators();
		}
	}
	//if (a.eventPhase == Event.BUBBLING_PHASE) { //@? @todo
	//	window.removeEventListener(PKEndEvent, this, false);

		//// old and shaky, doesn't work good with Xperia Play (and presumably lots of others)
		//if (!this->touchesHaveMoved && this->highlightItem.id >= 0) {
		//	_onSelect(this->highlightItem.id);
		//}
		if (Vec3(x, y, 0).distanceToSqr(startTouchPosition) <= 6.0f * 6.0f && this->highlightItem.id >= 0) {
			_onSelect(this->highlightItem.id);
		}
	//}
	te_moved = 0;
};


void ScrollingPane::touchesCancelled(float x, float y, int a) {
	touchesEnded(x, y, a);
}

void ScrollingPane::startDecelerationAnimation( bool force )
{
	Vec3 a(this->_contentOffset.x - this->startTimePosition.x, this->_contentOffset.y - this->startTimePosition.y, 0);
	float b = (getTimeMs()/*event.timeStamp*/ - this->startTime) / kAcceleration;
	//LOGI("starting deceleration! %s, %f\n", STR(a), b);

	this->decelerationVelocity = Vec3(a.x / b, a.y / b, 0);
	this->minDecelerationPoint = this->minPoint;
	this->maxDecelerationPoint = Vec3(0, 0, 0);
	if (this->pagingEnabled) {
		this->minDecelerationPoint.x = Mth::Max(this->minPoint.x, std::floor(this->_contentOffsetBeforeDeceleration.x / this->size.w) * this->size.w);
		this->minDecelerationPoint.y = Mth::Max(this->minPoint.y, std::floor(this->_contentOffsetBeforeDeceleration.y / this->size.h) * this->size.h);
		this->maxDecelerationPoint.x = Mth::Min(0.0f, std::ceil(this->_contentOffsetBeforeDeceleration.x / this->size.w) * this->size.w);
		this->maxDecelerationPoint.y = Mth::Min(0.0f, std::ceil(this->_contentOffsetBeforeDeceleration.y / this->size.h) * this->size.h);
	}
	this->penetrationDeceleration = kPenetrationDeceleration;
	this->penetrationAcceleration = kPenetrationAcceleration;
	if (this->pagingEnabled) {
		this->penetrationDeceleration *= 5;
	}
	float c = this->pagingEnabled ? kMinVelocityForDecelerationWithPaging : kMinVelocityForDeceleration;
	if (force || (Mth::abs(this->decelerationVelocity.x) > c || Mth::abs(this->decelerationVelocity.y) > c)) {
		this->decelerating = true;
		//LOGI("accelerating True - A\n");
		_doStepTimer = true;
		//this->decelerationTimer = this->callMethodNameAfterDelay("stepThroughDecelerationAnimation", kDesiredAnimationFrameRate); //@?
		this->lastFrame = getTimeMs();
		willBeginDecelerating();
	}
}

void ScrollingPane::hideScrollIndicators() {
	//hScroll.visible = vScroll.visible = false;
	hScroll.fading = vScroll.fading = 0;
}


void ScrollingPane::stopDecelerationAnimation()
{
	//LOGI("decelerating False - A\n");
	this->decelerating = false;
	_doStepTimer = false;
	//clearTimeout(this.decelerationTimer) //@?
}

void ScrollingPane::stepThroughDecelerationAnimation(bool noAnimation) {
	if (!this->decelerating) {
		return;
	}
	int d = getTimeMs();
	int k = d - this->lastFrame;

	int l = noAnimation ? 0 : (int)(std::floor(0.5f + ((float)k / kDesiredAnimationFrameRate) - 1));
	//LOGI("k: %d, %d %d : %d\n", d, this->lastFrame, k, l);
	for (int j = 0; j < l; j++)
		this->stepThroughDecelerationAnimation(true);

	float g = this->contentOffset().x + this->decelerationVelocity.x;
	float h = this->contentOffset().y + this->decelerationVelocity.y;
	if (isSet(SF_HardLimits)) {
		float a = Mth::Min(Mth::Max(this->minPoint.x, g), 0.0f);
		if (a != g) {
			g = a;
			this->decelerationVelocity.x = 0;
		}
		float c = Mth::Min(Mth::Max(this->minPoint.y, h), 0.0f);
		if (c != h) {
			h = c;
			this->decelerationVelocity.y = 0;
		}
	}
	if (noAnimation) {
		this->contentOffset().x = g;
		this->contentOffset().y = h;
	} else {
		this->setContentOffset(g, h);
	}
	if (!this->pagingEnabled) {
		this->decelerationVelocity.x *= kDecelerationFrictionFactor;
		this->decelerationVelocity.y *= kDecelerationFrictionFactor;
	}
	float b = Mth::abs(this->decelerationVelocity.x);
	float i = Mth::abs(this->decelerationVelocity.y);
	if (!noAnimation && b <= kMinimumVelocityToHideScrollIndicators && i <= kMinimumVelocityToHideScrollIndicators) {
		this->hideScrollIndicators();
		if (b <= kMinimumVelocity && i <= kMinimumVelocity) {
			//LOGI("decelerating False - B\n");
			this->decelerating = false;
			didEndDecelerating();
			return;
		}
	}
	if (!noAnimation) {
		//this->decelerationTimer = this->callMethodNameAfterDelay("stepThroughDecelerationAnimation", kDesiredAnimationFrameRate)
	}
	//if (noAnimation)  doStepTimer = false;


	if (isNotSet(SF_HardLimits)) {
		Vec3 e;
		if (g < this->minDecelerationPoint.x) {
			e.x = this->minDecelerationPoint.x - g;
		} else {
			if (g > this->maxDecelerationPoint.x) {
				e.x = this->maxDecelerationPoint.x - g;
			}
		}
		if (h < this->minDecelerationPoint.y) {
			e.y = this->minDecelerationPoint.y - h;
		} else {
			if (h > this->maxDecelerationPoint.y) {
				e.y = this->maxDecelerationPoint.y - h;
			}
		}
		if (e.x != 0) {
			if (e.x * this->decelerationVelocity.x <= 0) {
				this->decelerationVelocity.x += e.x * this->penetrationDeceleration;
			} else {
				this->decelerationVelocity.x = e.x * this->penetrationAcceleration;
			}
		}
		if (e.y != 0) {
			if (e.y * this->decelerationVelocity.y <= 0) {
				this->decelerationVelocity.y += e.y * this->penetrationDeceleration;
			} else {
				this->decelerationVelocity.y = e.y * this->penetrationAcceleration;
			}
		}
	}
	if (!noAnimation) {
		this->lastFrame = d;
	}
}

void ScrollingPane::setContentOffset(float x, float y) {
	this->setContentOffsetWithAnimation(Vec3(x, y, 0), false);
}
void ScrollingPane::setContentOffset(Vec3 a) {
	this->setContentOffsetWithAnimation(a, false);
};

void ScrollingPane::setContentOffsetWithAnimation(Vec3 b, bool doScroll) {
	this->_contentOffset = b;
	fpx = -this->_contentOffset.x;
	fpy = -this->_contentOffset.y;
	/* //@todo //@?
	this->hostingLayer.style.webkitTransform = PKUtils.t(this->_contentOffset.x, this->_contentOffset.y);
	if (a) {
		this->scrollTransitionsNeedRemoval = true;
		this->hostingLayer.style.webkitTransitionDuration = kPagingTransitionDuration
	} else {
		this->didScroll(false)
	}
	*/
	if (!doScroll) {
		// @todo: for scroll indicator //@?
		if (isSet(SF_ShowScrollbar)) {
			if (isNotSet(SF_LockX)) this->updateHorizontalScrollIndicator();
			if (isNotSet(SF_LockY)) this->updateVerticalScrollIndicator();
		}
	}
	//this->notifyPropertyChange("contentOffset")
}

void ScrollingPane::snapContentOffsetToBounds(bool a) {
	bool b = false;
	Vec3 c;
	if (this->pagingEnabled) {
		c.x = std::floor(0.5f + this->_contentOffset.x / this->size.w) * this->size.w;
		c.y = std::floor(0.5f + this->_contentOffset.y / this->size.h) * this->size.h;
		b = true;
	} else {
		if (isNotSet(SF_HardLimits)) {
			c.x = Mth::Min(Mth::Max(this->minPoint.x, this->_contentOffset.x), 0.0f);
			c.y = Mth::Min(Mth::Max(this->minPoint.y, this->_contentOffset.y), 0.0f);
			b = (c.x != this->_contentOffset.x || c.y != this->_contentOffset.y);
		}
	}
	if (b) {
		this->setContentOffsetWithAnimation(c, a);
	}
}

void ScrollingPane::adjustContentSize()
{
	this->adjustedContentSize.w = Mth::Max(itemBbox.w * columns, bbox.w);
	this->adjustedContentSize.h = Mth::Max(itemBbox.h * rows,    bbox.h);
}

void ScrollingPane::onHoldItem() {
	//LOGI("dragging, tracking %d, %d\n", !this->dragging, this->tracking);
	int id = highlightItem.id;
	if (id != -1 && !this->dragging && this->tracking)
		_onSelect(id);
	highlightTimer = -1;
	//highlightItem.id = -1;
}

bool ScrollingPane::onSelect( int gridId, bool selected )
{
	return selected;
}

void ScrollingPane::updateHorizontalScrollIndicator()
{
	float c = (isNotSet(SF_LockX) && isSet(SF_ShowScrollbar)) ? PKScrollIndicatorEndSize * 2 : 1;
	float d = Mth::Max(kMinIndicatorLength, std::floor(0.5f + (this->size.w / this->adjustedContentSize.w) * (this->size.w - c)));
	float a = (-this->_contentOffset.x / (this->adjustedContentSize.w - this->size.w)) * (this->size.w - c - d);
	//float b = this->size.h - PKScrollIndicatorThickness - 1;
	if (this->_contentOffset.x > 0) {
		d = std::floor(0.5f + Mth::Max(d - this->_contentOffset.x, PKScrollIndicatorThickness));
		a = 1;
	} else {
		if (this->_contentOffset.x < -(this->adjustedContentSize.w - this->size.w)) {
			d = std::floor(0.5f + Mth::Max(d + this->adjustedContentSize.w - this->size.w + this->contentOffset().x, PKScrollIndicatorThickness));
			a = this->size.w - d - c;
		}
	}
	this->hScroll.x = a + bbox.x;
	//this->hScroll.y = b;
	this->hScroll.w = d; //@property
};


void ScrollingPane::updateVerticalScrollIndicator()
{
	float c = (isNotSet(SF_LockY) && isSet(SF_ShowScrollbar)) ? PKScrollIndicatorEndSize * 2 : 1;
	float d = Mth::Max(kMinIndicatorLength, std::floor(0.5f + (this->size.h / this->adjustedContentSize.h) * (this->size.h - c)));
	//float a = this->size.w - PKScrollIndicatorThickness - 1;
	float b = (-this->_contentOffset.y / (this->adjustedContentSize.h - this->size.h)) * (this->size.h - c - d);
	if (this->_contentOffset.y > 0) {
		d = std::floor(0.5f + Mth::Max(d - this->_contentOffset.y, PKScrollIndicatorThickness));
		b = 1;
	} else {
		if (this->_contentOffset.y < -(this->adjustedContentSize.h - this->size.h)) {
			d = std::floor(0.5f + Mth::Max(d + this->adjustedContentSize.h - this->size.h + this->contentOffset().y, PKScrollIndicatorThickness));
			b = this->size.h - d - c;
		}
	}
	//this->vScroll.x = a;
	this->vScroll.y = b + bbox.y;
	this->vScroll.h = d; //@property
};

void ScrollingPane::_onSelect( int id )
{
	if (isSet(SF_MultiSelect)) {
		selected[id] = onSelect(id, !selected[id]);
	} else {
		// Change the selection, if the user wants it
		// @note: There's currently no way to clear a selection
		bool doSelect = onSelect(id, true);
		if (id != selectedId && doSelect) {
			onSelect(selectedId, false);
			selectedId = id;
		}
	}
}

void ScrollingPane::updateScrollFade( ScrollBar& sb ) {
	if (sb.fading == 1 && ((sb.alpha += 0.33f) >= 1)) {
		sb.alpha = 1;
		sb.fading = -1;
	}
	if (sb.fading == 0 && ((sb.alpha -= 0.10f) <= 0)) {
		sb.alpha = 0;
		sb.fading = -1;
	}
}

void ScrollingPane::setSelected( int id, bool selected )
{
	if (isSet(SF_MultiSelect))
		this->selected[id] = selected;
	else {
		if (selected) selectedId = selected? id : -1;
		else if (id == selectedId)
			selectedId = -1;
	}
}

void ScrollingPane::translate( float xo, float yo )
{
	bbox.x += (int)xo;
	bbox.y += (int)yo;
	area._x0 += xo;
	area._x1 += xo;
	area._y0 += yo;
	area._y1 += yo;
	bboxArea._x0 += xo;
	bboxArea._x1 += xo;
	bboxArea._y0 += yo;
	bboxArea._y1 += yo;
	hScroll.x += xo;
	hScroll.y += yo;
	vScroll.x += xo;
	vScroll.y += yo;
}

bool ScrollingPane::queryHoldTime(int* gridItem, int* heldMs) {
	*gridItem = -1;
	*heldMs = -1;

	if (!dragging && highlightStarted >= 0) {
		GridItem item = getItemForPos(_lx, _ly, true);
		if (item.id == highlightItem.id) {
			*gridItem = highlightItem.id;
			*heldMs = getTimeMs() - highlightStarted;
			return true;
		} else {
			highlightStarted = -1;
		}
	}

	return false;
}
