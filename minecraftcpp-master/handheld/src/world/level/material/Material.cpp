#include "Material.h"
#include "GasMaterial.h"
#include "LiquidMaterial.h"
#include "DecorationMaterial.h"
#include "WebMaterial.h"
#include <cstdlib>

const Material* Material::air		= NULL;
const Material* Material::dirt		= NULL;
const Material* Material::wood		= NULL;
const Material* Material::stone		= NULL;
const Material* Material::metal		= NULL;
const Material* Material::water		= NULL;
const Material* Material::lava		= NULL;
const Material* Material::leaves	= NULL;
const Material* Material::plant		= NULL;
const Material* Material::replaceable_plant = NULL;
const Material* Material::sponge	= NULL;
const Material* Material::cloth		= NULL;
const Material* Material::fire		= NULL;
const Material* Material::sand		= NULL;
const Material* Material::decoration= NULL;
const Material* Material::glass		= NULL;
const Material* Material::explosive = NULL;
const Material* Material::coral		= NULL;
const Material* Material::ice		= NULL;
const Material* Material::topSnow	= NULL;
const Material* Material::snow		= NULL;
const Material* Material::cactus	= NULL;
const Material* Material::clay		= NULL;
const Material* Material::vegetable = NULL;
const Material* Material::portal	= NULL;
const Material* Material::cake		= NULL;
const Material* Material::web		= NULL;

/*static*/
void Material::initMaterials()
{
	air       = new GasMaterial();
	dirt      = new Material();
	wood      = (new Material())->flammable();
	stone     = (new Material())->notAlwaysDestroyable();
	metal     = (new Material())->notAlwaysDestroyable();
	water     = new LiquidMaterial();
	lava      = new LiquidMaterial();
	leaves    = (new Material())->flammable()->neverBuildable();
	plant     = new DecorationMaterial();
	replaceable_plant = (new DecorationMaterial())->replaceable()->flammable();
	sponge    = new Material();
	cloth     = (new Material())->flammable();
	fire      = new GasMaterial();
	sand      = new Material();
	decoration= new DecorationMaterial();
	glass     = (new Material())->neverBuildable();
	explosive = (new Material())->flammable()->neverBuildable();
	coral     = new Material();
	ice       = (new Material())->neverBuildable();
	topSnow   = (new DecorationMaterial())->neverBuildable()->notAlwaysDestroyable()->replaceable();
	snow      = (new Material())->notAlwaysDestroyable();
	cactus    = (new Material())->neverBuildable();
	clay      = new Material();
	vegetable = new Material();
	portal    = new Material();
	cake      = new Material();
	web       = (new WebMaterial());
}

#define SAFEDEL(x) if (x) { delete x; x = NULL; }

/*static*/
void Material::teardownMaterials() {
	SAFEDEL(air);
	SAFEDEL(dirt);
	SAFEDEL(wood);
	SAFEDEL(stone);
	SAFEDEL(metal);
	SAFEDEL(water);
	SAFEDEL(lava);
	SAFEDEL(leaves);
	SAFEDEL(plant);
	SAFEDEL(replaceable_plant);
	SAFEDEL(sponge);
	SAFEDEL(cloth);
	SAFEDEL(fire);
	SAFEDEL(sand);
	SAFEDEL(decoration);
	SAFEDEL(glass);
	SAFEDEL(explosive);
	SAFEDEL(coral);
	SAFEDEL(ice);
	SAFEDEL(topSnow);
	SAFEDEL(snow);
	SAFEDEL(cactus);
	SAFEDEL(clay);
	SAFEDEL(vegetable);
	SAFEDEL(portal);
	SAFEDEL(cake);
	SAFEDEL(web);
}
