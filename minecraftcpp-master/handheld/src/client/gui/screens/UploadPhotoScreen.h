#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__UploadPhotoScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__UploadPhotoScreen_H__

#include "Screen.h"

class UploadPhotoScreen : public Screen
{
public:

	UploadPhotoScreen();
	virtual ~UploadPhotoScreen() {}

	virtual void init();

	void render(int xm, int ym, float a) {
		Screen::render(xm, ym, a);
	}

protected:

	virtual void mouseClicked(int x, int y, int buttonNum);
	virtual void mouseReleased(int x, int y, int buttonNum);
	virtual void keyPressed(int eventKey);

private:

	int selectedItem;

	void renderSlots();
	void renderSlot(int slot, int x, int y, float a);

	int getSelectedSlot(int x, int y);
	void selectSlotAndClose();
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__UploadPhotoScreen_H__*/
