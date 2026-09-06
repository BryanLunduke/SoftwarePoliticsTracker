#ifndef ITEMPANE_H__
#define ITEMPANE_H__

#include <string>
#include <vector>
#include "GuiElementContainer.h"
#include "../../../world/item/ItemInstance.h"
#include "../../../client/Options.h"
class Font; 
class Textures;
class NinePatchLayer;
class ItemPane;
class OptionButton;
class Button;
class OptionsGroup;
class Slider;
class Minecraft;
class OptionsPane: public GuiElementContainer
{
	typedef GuiElementContainer super;
public:
	OptionsPane();
	OptionsGroup& createOptionsGroup( std::string label );
	void createToggle( unsigned int group, std::string label, const Options::Option* option );
	void createProgressSlider(Minecraft* minecraft, unsigned int group, std::string label, const Options::Option* option, float progressMin=1.0f, float progressMax=1.0f );
	void createStepSlider(Minecraft* minecraft, unsigned int group, std::string label, const Options::Option* option, const std::vector<int>& stepVec );
	void setupPositions();
};

#endif /*ITEMPANE_H__*/
