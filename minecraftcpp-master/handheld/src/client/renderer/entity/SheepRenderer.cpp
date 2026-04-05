#include "SheepRenderer.h"
#include "../../../world/entity/animal/Sheep.h"
#include "../gles.h"

SheepRenderer::SheepRenderer( Model* model, Model* armor, float shadow )
:   super(model, shadow)
{
	setArmor(armor);
}

SheepRenderer::~SheepRenderer() {
	delete getArmor();
}

int SheepRenderer::prepareArmor(Mob* mob, int layer, float a) {
	Sheep* sheep = (Sheep*) mob;

	if (layer == 0 && !sheep->isSheared()) {
		bindTexture("mob/sheep_fur.png");

		float brightness = sheep->getBrightness(a);
		int color = sheep->getColor();
		glColor4f2(	brightness * Sheep::COLOR[color][0],
					brightness * Sheep::COLOR[color][1],
					brightness * Sheep::COLOR[color][2], 1);
		return 1;
	}
	return -1;
}
