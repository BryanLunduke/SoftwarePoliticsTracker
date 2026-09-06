#ifndef NET_MINECRAFT_CLIENT_PARTICLE__ParticleEngine_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__ParticleEngine_H__

//package net.minecraft.client.particle;

#include <vector>
#include "../../world/entity/Entity.h"
#include "../../world/level/tile/Tile.h"
#include "../renderer/gles.h"

class Textures;
class Level;
class Particle;
typedef std::vector<Particle*> ParticleList;

class ParticleEngine
{
public:
    static const int MISC_TEXTURE = 0;
    static const int TERRAIN_TEXTURE = 1;
    static const int ITEM_TEXTURE = 2;
    static const int ENTITY_PARTICLE_TEXTURE = 3;

    static const int TEXTURE_COUNT = 4;

    ParticleEngine(Level* level, Textures* textures);
	~ParticleEngine();

    void add(Particle* p);
	void destroy(int x, int y, int z);

    void tick();
    void render(Entity* player, float a);
    void renderLit(Entity* player, float a);

	void setLevel(Level* level);

	void crack(int x, int y, int z, int face);

	std::string countParticles();

protected:
	void clear();

	Level* level;
	GLuint textureIds[4];

private:
    ParticleList particles[4];
    Textures* textures;
    Random random;
};

#endif /*NET_MINECRAFT_CLIENT_PARTICLE__ParticleEngine_H__*/
