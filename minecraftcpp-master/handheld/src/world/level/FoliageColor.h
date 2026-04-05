#ifndef NET_MINECRAFT_WORLD_LEVEL__FoliageColor_H__
#define NET_MINECRAFT_WORLD_LEVEL__FoliageColor_H__

//package net.minecraft.world.level;

class FoliageColor
{
public:
//     static void init(int[] pixels) {
//         FoliageColor::pixels = pixels;
//     }
// 
//     static int get(float temp, float rain) {
//         rain *= temp;
//         int x = (int) ((1 - temp) * 255);
//         int y = (int) ((1 - rain) * 255);
//         return pixels[y << 8 | x];
//     }

    static int getEvergreenColor() {
        return 0x619961;
    }

    static int getBirchColor() {
        return 0x80a755;
    }

    static int getDefaultColor() {
        return 0x48b518;
    }

private:
    //static int pixels[256*256];
};

#endif /*NET_MINECRAFT_WORLD_LEVEL__FoliageColor_H__*/
