#ifndef NET_MINECRAFT_CLIENT_GUI__BaseContainerScreen_H__
#define NET_MINECRAFT_CLIENT_GUI__BaseContainerScreen_H__

//package net.minecraft.client.gui.screens;

#include <vector>
#include "../Screen.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"

class BaseContainerMenu;

class BaseContainerScreen: public Screen
{
    typedef Screen super;
public:
	BaseContainerScreen(BaseContainerMenu* menu)
	:  menu(menu)
    {
    }

	virtual void init() {
        super::init();
        minecraft->player->containerMenu = menu;
    }

	virtual void tick() {
		super::tick();
		if (!minecraft->player->isAlive() || minecraft->player->removed)
			minecraft->player->closeContainer();
	}

	virtual void keyPressed( int eventKey )
	{
		if (eventKey == Keyboard::KEY_ESCAPE) {
			minecraft->player->closeContainer();
		} else {
			super::keyPressed(eventKey);
		}
	}

	virtual bool closeOnPlayerHurt() {
		return true;
	}

protected:
	BaseContainerMenu* menu;
};

#endif /*NET_MINECRAFT_CLIENT_GUI__BaseContainerScreen_H__*/
