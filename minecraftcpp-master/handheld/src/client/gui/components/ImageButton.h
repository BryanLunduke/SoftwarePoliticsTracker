#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__ImageButton_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__ImageButton_H__

#include "Button.h"

typedef struct IntRectangle {
	IntRectangle()
	:	x(0),
		y(0),
		w(1),
		h(1)
	{}
	IntRectangle(int x, int y, int w, int h)
	:	x(x),
		y(y),
		w(w),
		h(h)
	{}

	int x, y;
	int w, h;
} IntRectangle;

typedef struct ImageDef {
	ImageDef()
	:	hasSrc(false),
		x(0),
		y(0),
		width(16),
		height(16)
		{}

	std::string name;
	int x;
	int y;
	float width;
	float height;

	ImageDef& setSrc(const IntRectangle& srcRect) {
		hasSrc = true;
		src = srcRect;
		return *this;
	}
	IntRectangle* getSrc() {
		return hasSrc? &src : NULL;
	}
protected:
	IntRectangle src;
	bool hasSrc;
} ImageDef;


class ImageButton: public Button
{
	typedef Button super;
public:
	ImageButton(int id, const std::string& msg);
	ImageButton(int id, const std::string& msg, const ImageDef& imageDef);
	void setImageDef(const ImageDef& imageDef, bool setButtonSize);

	void render(Minecraft* minecraft, int xm, int ym);
	void renderBg(Minecraft* minecraft, int xm, int ym) {}

protected:
	virtual void setupDefault();
	virtual bool isSecondImage(bool hovered) { return hovered; }

	ImageDef _imageDef;
public:
	bool scaleWhenPressed;
};

//
// A toggleable Button
//
class OptionButton: public ImageButton
{
	typedef ImageButton super;
public:
	OptionButton(const Options::Option* option);
	OptionButton(const Options::Option* option, float onValue, float offValue);

	void toggle(Options* options);
	void updateImage(Options* options);

	static const int ButtonId = 9999999;
protected:
	bool isSecondImage(bool hovered);

	virtual void mouseClicked( Minecraft* minecraft, int x, int y, int buttonNum );

private:

	const Options::Option* _option;
	bool _secondImage;

	// If not float, it's considered to be a boolean value
	bool _isFloat;
	float _onValue;
	float _offValue;
	float _current;
};


#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__ImageButton_H__*/
