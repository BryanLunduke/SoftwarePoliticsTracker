#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__TextBox_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__TextBox_H__

//package net.minecraft.client.gui;

#include <string>
#include "../GuiComponent.h"
#include "../../Options.h"

class Font;
class Minecraft;

class TextBox: public GuiComponent
{
public:
	TextBox(int id, const std::string& msg);
 TextBox(int id, int x, int y, const std::string& msg);
 TextBox(int id, int x, int y, int w, int h, const std::string& msg);

	virtual void setFocus(Minecraft* minecraft);
	virtual bool loseFocus(Minecraft* minecraft);

 virtual void render(Minecraft* minecraft, int xm, int ym);

	// Editing helpers
	int getCaretPos() const { return caretPos; }
	void setCaretPos(int p);
	void insertChar(char c);
	void backspaceAtCaret();
	void deleteAtCaret();
	void moveCaretLeft();
	void moveCaretRight();
	void moveCaretHome();
	void moveCaretEnd();
	void setCaretFromPixel(int px, Font* font);

public:
	int w, h;
	int x, y;

	std::string text;
	int id;
	bool focused;
private:
	int caretPos; // character index
};

#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__TextBox_H__*/
