#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__RenameMPLevelScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__RenameMPLevelScreen_H__

#include "../Screen.h"

class RenameMPLevelScreen: public Screen
{
public:
    RenameMPLevelScreen(const std::string& levelId);

    virtual void init();
	virtual void render(int xm, int ym, float a);

private:
    std::string _levelId;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__RenameMPLevelScreen_H__*/