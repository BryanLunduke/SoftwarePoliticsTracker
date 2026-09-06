#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__LargeCaveFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__LargeCaveFeature_H__

//package net.minecraft.world.level.levelgen;

#include "../../../util/Random.h"
#include "../../../util/Mth.h"

#include "LargeFeature.h"

#include "../Level.h"
#include "../tile/Tile.h"
#include "../tile/GrassTile.h"

class LargeCaveFeature: public LargeFeature
{
protected:
    void addRoom(int xOffs, int zOffs, unsigned char* blocks, float xRoom, float yRoom, float zRoom) {
        addTunnel(xOffs, zOffs, blocks, xRoom, yRoom, zRoom, 1 + random.nextFloat() * 6, 0, 0, -1, -1, 0.5);
    }

    void addTunnel(int xOffs, int zOffs, unsigned char* blocks, float xCave, float yCave, float zCave, float thickness, float yRot, float xRot, int step, int dist, float yScale) {
        float xMid = (float)(xOffs * 16 + 8);
        float zMid = (float)(zOffs * 16 + 8);

        float yRota = 0;
        float xRota = 0;
        Random random(this->random.nextLong());

        if (dist <= 0) {
            int max = radius * 16 - 16;
            dist = max - random.nextInt(max / 4);
        }
        bool singleStep = false;

        if (step == -1) {
            step = dist / 2;
            singleStep = true;
        }


        int splitPoint = random.nextInt(dist / 2) + dist / 4;
        bool steep = random.nextInt(6) == 0;

        for (; step < dist; step++) {
            float rad = 1.5f + (sin(step * Mth::PI / dist) * thickness) * 1;
            float yRad = rad * yScale;

            float xc = cos(xRot);
            float xs = sin(xRot);
            xCave += cos(yRot) * xc;
            yCave += xs;
            zCave += sin(yRot) * xc;

            if (steep) {
                xRot *= 0.92f;
            } else {
                xRot *= 0.7f;
            }
            xRot += xRota * 0.1f;
            yRot += yRota * 0.1f;

            xRota *= 0.90f;
            yRota *= 0.75f;
            xRota += (random.nextFloat() - random.nextFloat()) * random.nextFloat() * 2;
            yRota += (random.nextFloat() - random.nextFloat()) * random.nextFloat() * 4;


            if (!singleStep && step == splitPoint && thickness > 1) {
                addTunnel(xOffs, zOffs, blocks, xCave, yCave, zCave, random.nextFloat() * 0.5f + 0.5f, yRot - Mth::PI / 2, xRot / 3, step, dist, 1.0);
                addTunnel(xOffs, zOffs, blocks, xCave, yCave, zCave, random.nextFloat() * 0.5f + 0.5f, yRot + Mth::PI / 2, xRot / 3, step, dist, 1.0);
                return;
            }
            if (!singleStep && random.nextInt(4) == 0) continue;

            {
                float xd = xCave - xMid;
                float zd = zCave - zMid;
                float remaining = (float)(dist - step);
                float rr = (thickness + 2) + 16;
                if (xd * xd + zd * zd - (remaining * remaining) > rr * rr) {
                    return;
                }
            }

            if (xCave < xMid - 16 - rad * 2 || zCave < zMid - 16 - rad * 2 || xCave > xMid + 16 + rad * 2 || zCave > zMid + 16 + rad * 2) continue;

            int x0 = (int)floor(xCave - rad) - xOffs * 16 - 1;
            int x1 = (int)floor(xCave + rad) - xOffs * 16 + 1;

            int y0 = (int)floor(yCave - yRad) - 1;
            int y1 = (int)floor(yCave + yRad) + 1;

            int z0 = (int)floor(zCave - rad) - zOffs * 16 - 1;
            int z1 = (int)floor(zCave + rad) - zOffs * 16 + 1;

            if (x0 < 0) x0 = 0;
            if (x1 > 16) x1 = 16;

            if (y0 < 1) y0 = 1;
            if (y1 > 120) y1 = 120;

            if (z0 < 0) z0 = 0;
            if (z1 > 16) z1 = 16;

            bool detectedWater = false;
            for (int xx = x0; !detectedWater && xx < x1; xx++) {
                for (int zz = z0; !detectedWater && zz < z1; zz++) {
                    for (int yy = y1 + 1; !detectedWater && yy >= y0 - 1; yy--) {
                        int p = (xx * 16 + zz) * 128 + yy;
                        if (yy < 0 || yy >= Level::DEPTH) continue;
                        if (blocks[p] == Tile::water->id || blocks[p] == Tile::calmWater->id) {
                            detectedWater = true;
                        }
                        if (yy != y0 - 1 && xx != x0 && xx != x1 - 1 && zz != z0 && zz != z1 - 1) {
                            yy = y0;
                        }
                    }
                }
            }
            if (detectedWater) continue;

            for (int xx = x0; xx < x1; xx++) {
                float xd = ((xx + xOffs * 16 + 0.5f) - xCave) / rad;
                for (int zz = z0; zz < z1; zz++) {
                    float zd = ((zz + zOffs * 16 + 0.5f) - zCave) / rad;
                    int p = (xx * 16 + zz) * 128 + y1;
                    bool hasGrass = false;
                    if (xd * xd + zd * zd < 1) {
                        for (int yy = y1 - 1; yy >= y0; yy--) {
                            float yd = (yy + 0.5f - yCave) / yRad;
                            if (yd > -0.7 && xd * xd + yd * yd + zd * zd < 1) {
                                int block = blocks[p];
                                if (block == Tile::grass->id) hasGrass = true;
                                if (block == Tile::rock->id || block == Tile::dirt->id || block == Tile::grass->id) {
                                    if (yy < 10) {
                                        blocks[p] = (unsigned char) Tile::lava->id;
                                    } else {
                                        blocks[p] = (unsigned char) 0;
                                        if (hasGrass && blocks[p - 1] == Tile::dirt->id) blocks[p - 1] = (unsigned char) Tile::grass->id;
                                    }
                                }
                            }
                            p--;
                        }
                    }
                }
            }
            if (singleStep) break;
        }
    }

    void addFeature(Level* level, int x, int z, int xOffs, int zOffs, unsigned char* blocks, int blocksSize) {
        int caves = random.nextInt(random.nextInt(random.nextInt(40) + 1) + 1);
        if (random.nextInt(15) != 0) caves = 0;

        for (int cave = 0; cave < caves; cave++) {
            float xCave = (float)(x * 16 + random.nextInt(16));
            float yCave = (float)(random.nextInt(random.nextInt(120) + 8));
            float zCave = (float)(z * 16 + random.nextInt(16));

            int tunnels = 1;
            if (random.nextInt(4) == 0) {
                addRoom(xOffs, zOffs, blocks, xCave, yCave, zCave);
                tunnels += random.nextInt(4);
            }

            for (int i = 0; i < tunnels; i++) {

                float yRot = random.nextFloat() * Mth::PI * 2;
                float xRot = ((random.nextFloat() - 0.5f) * 2) / 8;
                float thickness = random.nextFloat() * 2 + random.nextFloat();

                addTunnel(xOffs, zOffs, blocks, xCave, yCave, zCave, thickness, yRot, xRot, 0, 0, 1.0);
            }
        }
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__LargeCaveFeature_H__*/
