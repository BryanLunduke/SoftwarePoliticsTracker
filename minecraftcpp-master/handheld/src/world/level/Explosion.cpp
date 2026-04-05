#include "Explosion.h"

#include "Level.h"
#include "tile/Tile.h"
#include "../entity/Entity.h"


Explosion::Explosion(Level* level, Entity* source, float x, float y, float z, float r)
:	level(level),
	source(source),
	x(x),
	y(y),
	z(z),
	r(r),
	fire(false)
{
}

void Explosion::explode()
{
	float org = r;

	int size = 16;
	for (int xx = 0; xx < size; xx++)
		for (int yy = 0; yy < size; yy++)
			for (int zz = 0; zz < size; zz++) {
				if ((xx != 0 && xx != size - 1) && (yy != 0 && yy != size - 1) && (zz != 0 && zz != size - 1)) continue;

				float xd = xx / (size - 1.0f) * 2 - 1;
				float yd = yy / (size - 1.0f) * 2 - 1;
				float zd = zz / (size - 1.0f) * 2 - 1;
				float d = sqrt(xd * xd + yd * yd + zd * zd);

				xd /= d;
				yd /= d;
				zd /= d;

				float remainingPower = r * (0.7f + level->random.nextFloat() * 0.6f);
				float xp = x;
				float yp = y;
				float zp = z;

				float stepSize = 0.3f;
				while (remainingPower > 0) {
					int xt = Mth::floor(xp);
					int yt = Mth::floor(yp);
					int zt = Mth::floor(zp);
					int t = level->getTile(xt, yt, zt);
					if (t > 0) {
						remainingPower -= (Tile::tiles[t]->getExplosionResistance(source) + 0.3f) * stepSize;
					}
					if (remainingPower > 0) {
						toBlow.insert(TilePos(xt, yt, zt));
					}

					xp += xd * stepSize;
					yp += yd * stepSize;
					zp += zd * stepSize;
					remainingPower -= stepSize * 0.75f;
				}
			}

			r += r;
			int x0 = Mth::floor(x - r - 1);
			int x1 = Mth::floor(x + r + 1);
			int y0 = Mth::floor(y - r - 1);
			int y1 = Mth::floor(y + r + 1);
			int z0 = Mth::floor(z - r - 1);
			int z1 = Mth::floor(z + r + 1);
			EntityList& entities = level->getEntities(source, AABB((float)x0, (float)y0, (float)z0, (float)x1, (float)y1, (float)z1));
			Vec3 center(x, y, z);
			for (unsigned int i = 0; i < entities.size(); i++) {
				Entity* e = entities[i];
				float dist = e->distanceTo(x, y, z) / r;
				if (dist <= 1) {
					float xa = e->x - x;
					float ya = e->y - y;
					float za = e->z - z;

					float ida = Mth::invSqrt(xa * xa + ya * ya + za * za);

					xa *= ida;
					ya *= ida;
					za *= ida;

					float sp = level->getSeenPercent(center, e->bb);
					float pow = (1 - dist) * sp;
					e->hurt(source, (int) ((pow * pow + pow) / 2 * 8 * r + 1));

					float push = pow;
					e->xd += xa * push;
					e->yd += ya * push;
					e->zd += za * push;
				}
			}
			r = org;

			std::vector<TilePos> toBlowArray;
			toBlowArray.insert(toBlowArray.end(), toBlow.begin(), toBlow.end());

			if (fire) {
				for (int j = (int)toBlowArray.size() - 1; j >= 0; j--) {
					const TilePos& tp = toBlowArray[j];
					int xt = tp.x;
					int yt = tp.y;
					int zt = tp.z;
					int t = level->getTile(xt, yt, zt);
					int b = level->getTile(xt, yt - 1, zt);
					if (t == 0 && Tile::solid[b] && random.nextInt(3) == 0) {
						level->setTileNoUpdate(xt, yt, zt, ((Tile*)Tile::fire)->id);
					}
				}
			}
}

void Explosion::finalizeExplosion()
{
	level->playSound(x, y, z, "random.explode", 4, (1 + (level->random.nextFloat() - level->random.nextFloat()) * 0.2f) * 0.7f);
	//level->addParticle(PARTICLETYPE(hugeexplosion), x, y, z, 0, 0, 0);

	int j = 0;
	for (TilePosSet::const_iterator cit = toBlow.begin(); cit != toBlow.end(); ++cit, ++j) {
		const TilePos& tp = *cit;
		int xt = tp.x;
		int yt = tp.y;
		int zt = tp.z;
		int t = level->getTile(xt, yt, zt);

		do {
			if (j & 7) break;

			float xa = xt + level->random.nextFloat();
			float ya = yt + level->random.nextFloat();
			float za = zt + level->random.nextFloat();

			float xd = xa - x;
			float yd = ya - y;
			float zd = za - z;

			float invdd = 1.0f / Mth::sqrt(xd * xd + yd * yd + zd * zd);

			xd *= invdd;
			yd *= invdd;
			zd *= invdd;

			float speed = 0.5f / (r / invdd + 0.1f);
			speed *= (level->random.nextFloat() * level->random.nextFloat() + 0.3f);
			xd *= speed;
			yd *= speed;
			zd *= speed;

			level->addParticle(PARTICLETYPE(explode), (xa + x * 1) / 2, (ya + y * 1) / 2, (za + z * 1) / 2, xd, yd, zd);
			level->addParticle(PARTICLETYPE(smoke), xa, ya, za, xd, yd, zd);
		} while (0);

		if (t > 0) {
			if (!level->isClientSide) Tile::tiles[t]->spawnResources(level, xt, yt, zt, level->getData(xt, yt, zt), 0.3f);
			if (level->setTileNoUpdate(xt, yt, zt, 0))
				level->updateNeighborsAt(xt, yt, zt, 0);
			level->setTileDirty(xt, yt, zt);
			if (!level->isClientSide) Tile::tiles[t]->wasExploded(level, xt, yt, zt);
		}
	}
}
