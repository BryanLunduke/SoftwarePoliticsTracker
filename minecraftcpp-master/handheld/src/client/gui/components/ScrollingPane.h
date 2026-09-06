#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__ScrollingPane_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__ScrollingPane_H__

#include "../GuiComponent.h"
#include "ImageButton.h"
#include "../../player/input/touchscreen/TouchAreaModel.h"
#include "../../../world/phys/Vec3.h"
#include "../../Timer.h"

enum ScrollingPaneFlags {
	SF_LockX		= 1 << 0,
	SF_LockY		= 1 << 1,
	SF_WrapX		= 1 << 2,
	SF_WrapY		= 1 << 3,
	SF_HardLimits	= 1 << 4,
	SF_MultiSelect	= 1 << 5,
	//SF_Snap		= 1 << 6,
	//SF_CustomSnap = 1 << 7,
	//SF_Scissor	= 1 << 8,
	SF_ShowScrollbar= 1 << 9,
	SF_NoHoldSelect = 1 << 10
};

typedef struct ScrollBar {
	ScrollBar()
	:	alpha(0),
		fading(-1)
	{}
	float x;
	float y;
	float w;
	float h;
	//bool visible;
	float alpha;
	int   fading;
} ScrollBar;

class ScrollingPane: public GuiComponent {
public:
	typedef struct GridItem {
		int id;
		int x, y;
		// The GUI coordinates comes in (xf, yf)
		float xf, yf;
		bool selected;
	} GridItem;

	ScrollingPane(int flags, const IntRectangle& boundingBox, const IntRectangle& itemRect, int columns, int numItems, float screenScale = 1.0f, const IntRectangle& itemBoundingRect = IntRectangle(0,0,0,0));
	~ScrollingPane();
	//void init(Minecraft*, int width, int height);
	void tick();
	void render(int xm, int ym, float alpha);

	bool getGridItemFor_slow(int itemIndex, GridItem& out);

	void setSelected(int id, bool selected);

	// This function is called with all visible GridItems. The base
	// implementation just dispatches each item to renderItem in y,x order
	virtual void renderBatch(std::vector<GridItem>& items, float alpha);
	virtual void renderItem(GridItem& item, float alpha);

	//void render(int xx, int yy);
	bool queryHoldTime(int* gridId, int* heldMs);

protected:
	GridItem getItemForPos(float x, float y, bool isScreenPos);
	void handleUserInput();
	void addDeltaPos(float x, float y, float dt, int z);

	void translate(float xo, float yo);

	int flags;

	int columns;
	int rows;
	int numItems;

	int px, py;
	float fpx, fpy;

	float screenScale;
	float invScreenScale;
	//bool hasItemBounding;

	IntRectangle bbox;
	IntRectangle itemRect;
	IntRectangle itemBbox;
	RectangleArea area;
	RectangleArea bboxArea;

	// Dragging info
	std::vector<float>	dragDeltas;
	int					dragState;
	Vec3				dragBeginPos;
	Vec3				dragBeginScreenPos;
	int					dragTicks;
	float				dragLastDeltaTimeStamp;
	Vec3				dragLastPos;

	float dx, dy;
	float friction;

	float dstx, dsty;

	// Rewrite
	bool dragging; //!
	bool decelerating;
	bool tracking; //!

	bool pagingEnabled; //!

	Vec3 _contentOffset; //!
	Vec3 _contentOffsetBeforeDeceleration; //*

	int lastEventTime; //<

	Vec3 decelerationVelocity; //*
	Vec3 minDecelerationPoint; //*
	Vec3 maxDecelerationPoint; //*

	float penetrationDeceleration; //<
	float penetrationAcceleration; //<

	Vec3 minPoint; //*
	Vec3 startPosition; //*
	Vec3 startTouchPosition; //*
	Vec3 startTimePosition; //*

	bool wasDeceleratingWhenTouchesBegan; //*
	bool firstDrag; //<

	float startTime; //<
	//float startTime

	IntRectangle size;
	int lastFrame;

	bool _scrollEnabled; //!
	bool touchesHaveMoved; //*

	virtual void didEndDragging() {}
	virtual void didEndDecelerating() {}
	virtual void willBeginDecelerating() {}
	virtual void willBeginDragging() {}

	int te_moved,
		te_ended,
		te_highlight;
	int highlightTimer;
	int highlightStarted;
	GridItem highlightItem;

	bool* selected;
	int selectedId;

	ScrollBar vScroll, hScroll;

	IntRectangle adjustedContentSize;
	void touchesBegan(float x, float y, int t);
	void touchesMoved(float x, float y, int t);
	void touchesEnded(float x, float y, int t);
	void touchesCancelled(float x, float y, int t);
	void beginTracking(float x, float y, int t);
	void onHoldItem();

	void _onSelect( int id );
	virtual bool onSelect(int gridId, bool selected); 

	Vec3& contentOffset();

	void startDecelerationAnimation(bool force);
	void stopDecelerationAnimation();
	void stepThroughDecelerationAnimation(bool f);

	void setContentOffset(float x, float y);
	void setContentOffset(Vec3 a);
	void setContentOffsetWithAnimation(Vec3 b, bool doScroll);
	void snapContentOffsetToBounds(bool snap); //*
	void adjustContentSize();
	//TouchAreaModel _areaModel;

	bool isAllSet(int flag) { return (flags & flag) == flag; }
	bool isSet(int flag)    { return (flags & flag) != 0; }
	bool isNotSet(int flag) { return !isSet(flag); }

	void updateHorizontalScrollIndicator();
	void updateVerticalScrollIndicator();
	void hideScrollIndicators(); //*
	void updateScrollFade( ScrollBar& vScroll );
private:
	Timer _timer;
	bool  _doStepTimer;
	bool  _wasDown;
	float _lx;
	float _ly;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__ScrollingPane_H__*/
