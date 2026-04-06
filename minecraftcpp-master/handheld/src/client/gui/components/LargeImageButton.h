#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__LargeImageButton_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__LargeImageButton_H__

#include "ImageButton.h"

class LargeImageButton: public ImageButton
{
	typedef ImageButton super;
public:
	LargeImageButton(int id, const std::string& msg);
	LargeImageButton(int id, const std::string& msg, ImageDef& imageDef);

	void render(Minecraft* minecraft, int xm, int ym);

private:
	void setupDefault();

	float _buttonScale;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__LargeImageButton_H__*/
