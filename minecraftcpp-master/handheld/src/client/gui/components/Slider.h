#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__Slider_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__Slider_H__

#include "GuiElement.h"
#include "../../../client/Options.h"
enum SliderType {
	SliderProgress, // Sets slider between {0..1}
	SliderStep // Uses the closest step
};
class Slider : public GuiElement {
	typedef GuiElement super;
public:
	// Creates a progress slider with no steps
	Slider(Minecraft* minecraft, const Options::Option* option, float progressMin, float progressMax);
	Slider(Minecraft* minecraft, const Options::Option* option, const std::vector<int>& stepVec);
	virtual void render( Minecraft* minecraft, int xm, int ym );

	virtual void mouseClicked( Minecraft* minecraft, int x, int y, int buttonNum );

	virtual void mouseReleased( Minecraft* minecraft, int x, int y, int buttonNum );

	virtual void tick(Minecraft* minecraft);
	
private:
	virtual void setOption(Minecraft* minecraft);

private:
	SliderType sliderType;
	std::vector<int> sliderSteps;
	bool mouseDownOnElement;
	float percentage;
	int curStepValue;
	int curStep;
	int numSteps;
	float progressMin;
	float progressMax;
	const Options::Option* option;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__Slider_H__*/
