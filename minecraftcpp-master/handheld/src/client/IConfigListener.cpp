#include "IConfigListener.h"
#include "Minecraft.h"
#ifndef STANDALONE_SERVER
#include "gui/Gui.h"
#endif /* STANDALONE_SERVER */
Config createConfig(Minecraft* mc) {
	Config c;
	#ifndef STANDALONE_SERVER
	c.setScreenSize(mc->width, mc->height, Gui::GuiScale);
	#endif
	c.pixelCalc   = mc->pixelCalc;
	c.pixelCalcUi = mc->pixelCalcUi;
	c.minecraft = mc;
	c.options   = &mc->options;
	return c;
}
