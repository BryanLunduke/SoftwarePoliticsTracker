#include "ChickenRenderer.h"
#include "../../../util/Mth.h"
#include "../../../world/entity/animal/Chicken.h"


ChickenRenderer::ChickenRenderer( Model* model, float shadow )
:   super(model, shadow)
{
}

void ChickenRenderer::render( Entity* mob, float x, float y, float z, float rot, float a )
{
	super::render(mob, x, y, z, rot, a);
}

float ChickenRenderer::getBob( Mob* mob_, float a )
{
	Chicken* mob = (Chicken*) mob_;
	float flap = mob->oFlap+(mob->flap-mob->oFlap)*a;
	float flapSpeed = mob->oFlapSpeed+(mob->flapSpeed-mob->oFlapSpeed)*a;

	return (Mth::sin(flap)+1)*flapSpeed;
}
