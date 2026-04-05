#include "JoinGameScreen.h"
#include "StartMenuScreen.h"
#include "ProgressScreen.h"
#include "../Font.h"
#include "../../../network/RakNetInstance.h"
#include "../../../network/ClientSideNetworkHandler.h"

#include "../components/TextBox.h"

#include <string>
#include <cctype>
#include <cstdlib>

JoinGameScreen::JoinGameScreen()
: bJoin(2, "Join Game"),
 bBack(3, "Back"),
 ip(5, "IP+Port?")
{
 bJoin.active = true; // enable manual join
}

JoinGameScreen::~JoinGameScreen()
{
}

void JoinGameScreen::buttonClicked(Button* button)
{
 if (button->id == bJoin.id)
 {
 std::string addr = ip.text;
 auto trim = [](std::string &s) {
 size_t start =0;
 while (start < s.size() && std::isspace((unsigned char)s[start])) ++start;
 size_t end = s.size();
 while (end > start && std::isspace((unsigned char)s[end -1])) --end;
 s = s.substr(start, end - start);
 };
 trim(addr);
 if (!addr.empty() && addr != "IP+Port?") {
 std::string host = addr;
 int port =19132;
 size_t colon = addr.find(':');
 if (colon != std::string::npos) {
 host = addr.substr(0, colon);
 std::string portStr = addr.substr(colon +1);
 trim(host);
 trim(portStr);
 if (!portStr.empty()) port = std::atoi(portStr.c_str());
 } else trim(host);
 if (!host.empty()) {
 if (minecraft->netCallback) { delete minecraft->netCallback; minecraft->netCallback = NULL; }
 minecraft->netCallback = new ClientSideNetworkHandler(minecraft, minecraft->raknetInstance);
 minecraft->raknetInstance->connect(host.c_str(), port);
 bJoin.active = false;
 bBack.active = false;
 minecraft->setScreen(new ProgressScreen());
 }
 }
 }
 if (button->id == bBack.id)
 {
 minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
 }
}

bool JoinGameScreen::handleBackEvent(bool isDown)
{
 if (!isDown) minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
 return true;
}

bool JoinGameScreen::isIndexValid( int index ) { return false; }

void JoinGameScreen::tick()
{
}

void JoinGameScreen::init()
{
 buttons.push_back(&bJoin);
 buttons.push_back(&bBack);
 textBoxes.push_back(&ip);
}

void JoinGameScreen::setupPositions() {
 int yBase = height -26;
 bJoin.y = yBase;
 bBack.y = yBase;
 bBack.width = bJoin.width =120;
 bJoin.x = width /2 -4 - bJoin.width;
 bBack.x = width /2 +4;
 ip.x = width/2 -60; // center
 ip.y =10;
 ip.w =120;
}

void JoinGameScreen::render( int xm, int ym, float a )
{
 renderBackground();
 // Render textboxes explicitly (Screen::render only draws buttons)
 for (unsigned int i =0; i < textBoxes.size(); ++i) {
 textBoxes[i]->render(minecraft, xm, ym);
 }
 Screen::render(xm, ym, a);
}

bool JoinGameScreen::isInGameScreen() { return false; }
