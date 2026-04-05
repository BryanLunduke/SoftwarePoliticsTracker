#include "Motive.h"
const Motive Motive::Kebab("Kebab", 16, 16, 0 * 16, 0 * 16);
const Motive Motive::Aztec("Aztec", 16, 16, 1 * 16, 0 * 16);
const Motive Motive::Alban("Alban", 16, 16, 2 * 16, 0 * 16);
const Motive Motive::Aztec2("Aztec2", 16, 16, 3 * 16, 0 * 16);
const Motive Motive::Bomb("Bomb", 16, 16, 4 * 16, 0 * 16);
const Motive Motive::Plant("Plant", 16, 16, 5 * 16, 0 * 16);
const Motive Motive::Wasteland("Wasteland", 16, 16, 6 * 16, 0 * 16);
const Motive Motive::Pool("Pool", 32, 16, 0 * 16, 2 * 16);
const Motive Motive::Courbet("Courbet", 32, 16, 2 * 16, 2 * 16);
const Motive Motive::Sea("Sea", 32, 16, 4 * 16, 2 * 16);
const Motive Motive::Sunset("Sunset", 32, 16, 6 * 16, 2 * 16);
const Motive Motive::Creebet("Creebet", 32, 16, 8 * 16, 2 * 16);
const Motive Motive::Wanderer("Wanderer", 16, 32, 0 * 16, 4 * 16);
const Motive Motive::Graham("Graham", 16, 32, 1 * 16, 4 * 16);
const Motive Motive::Match("Match", 32, 32, 0 * 16, 8 * 16);
const Motive Motive::Bust("Bust", 32, 32, 2 * 16, 8 * 16);
const Motive Motive::Stage("Stage", 32, 32, 4 * 16, 8 * 16);
const Motive Motive::Void("Void", 32, 32, 6 * 16, 8 * 16);
const Motive Motive::SkullAndRoses("SkullAndRoses", 32, 32, 8 * 16, 8 * 16);
const Motive Motive::Fighters("Fighters", 64, 32, 0 * 16, 6 * 16);
const Motive Motive::Pointer("Pointer", 64, 64, 0 * 16, 12 * 16);
const Motive Motive::Pigscene("Pigscene", 64, 64, 4 * 16, 12 * 16);
const Motive Motive::BurningSkull("BurningSkull", 64, 64, 8 * 16, 12 * 16);
const Motive Motive::Skeleton("Skeleton", 64, 48, 12 * 16, 4 * 16);
const Motive Motive::DonkeyKong("DonkeyKong", 64, 48, 12 * 16, 7 * 16);
const Motive Motive::Earth("Earth", 32, 32, 0 * 16, 10 * 16, false);
const Motive Motive::Wind("Wind", 32, 32, 2 * 16, 10 * 16, false);
const Motive Motive::Fire("Fire", 32, 32, 4 * 16, 10 * 16, false);
const Motive Motive::Water("Water", 32, 32, 6 * 16, 10 * 16, false);
const Motive* Motive::DefaultImage = &Motive::Kebab;

std::vector<const Motive*> Motive::getAllMotivesAsList() {
	std::vector<const Motive*> motives;
	motives.push_back(&Kebab);
	motives.push_back(&Aztec2);
	motives.push_back(&Alban);
	motives.push_back(&Bomb);
	motives.push_back(&Plant);
	motives.push_back(&Wasteland);
	motives.push_back(&Pool);
	motives.push_back(&Courbet);
	motives.push_back(&Sea);
	motives.push_back(&Sunset);
	motives.push_back(&Creebet);
	motives.push_back(&Wanderer);
	motives.push_back(&Graham);
	motives.push_back(&Match);
	motives.push_back(&Bust);
	motives.push_back(&Stage);
	motives.push_back(&Void);
	motives.push_back(&SkullAndRoses);
	motives.push_back(&Fighters);
	motives.push_back(&Pointer);
	motives.push_back(&Pigscene);
	motives.push_back(&BurningSkull);
	motives.push_back(&Skeleton);
	motives.push_back(&DonkeyKong);

	motives.push_back(&Earth);
	motives.push_back(&Wind);
	motives.push_back(&Fire);
	motives.push_back(&Water);

	return motives;
}

const Motive* Motive::getMotiveByName( const std::string& name ) {
	std::vector<const Motive*> allMovies = getAllMotivesAsList();
	for(std::vector<const Motive*>::iterator i = allMovies.begin(); i != allMovies.end(); ++i) {
		if((*i)->name == name)
			return *i;
	}
	return DefaultImage;
}

Motive::Motive( std::string name, int w, int h, int uo, int vo, bool isPublic /*= true*/ )
:	name(name),
	w(w),
	h(h),
	uo(uo),
	vo(vo),
	isPublic(isPublic)
{}
