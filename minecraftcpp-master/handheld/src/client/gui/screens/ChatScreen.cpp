#include "ChatScreen.h"
#include "DialogDefinitions.h"
#include "../Gui.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "../../../platform/log.h"

void ChatScreen::init() {
	minecraft->platform()->createUserInput(DialogDefinitions::DIALOG_NEW_CHAT_MESSAGE);
}

void ChatScreen::render(int xm, int ym, float a)
{
	int status = minecraft->platform()->getUserInputStatus();
	if (status > -1) {
		if (status == 1) {
			std::vector<std::string> v = minecraft->platform()->getUserInput();
			if (v.size() && v[0].length() > 0)
				minecraft->gui.addMessage(v[0]);
		}

		minecraft->setScreen(NULL);
	}
}
