#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__SmallButton_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__SmallButton_H__

//package net.minecraft.client.gui;

#include <string>
#include "Button.h"
#include "../../Options.h"

class SmallButton: public Button
{
	typedef Button super;
public:
    SmallButton(int id, int x, int y, const std::string& msg);
    SmallButton(int id, int x, int y, int width, int height, const std::string& msg);
    SmallButton(int id, int x, int y, Options::Option* item, const std::string& msg);

    Options::Option* getOption();
private:
	Options::Option* option;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__SmallButton_H__*/
