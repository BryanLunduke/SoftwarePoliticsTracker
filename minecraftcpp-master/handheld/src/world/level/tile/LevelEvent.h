#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__LevelEvent_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__LevelEvent_H__

class LevelEvent
{
public:
    static const int SOUND_CLICK = 1000;
    static const int SOUND_CLICK_FAIL = 1001;
    static const int SOUND_LAUNCH = 1002;
    static const int SOUND_OPEN_DOOR = 1003;
    static const int SOUND_FIZZ = 1004;
    static const int SOUND_PLAY_RECORDING = 1005;

    static const int SOUND_GHAST_WARNING = 1007;
    static const int SOUND_GHAST_FIREBALL = 1008;
    static const int SOUND_BLAZE_FIREBALL = 1009;

    static const int PARTICLES_SHOOT = 2000;
    static const int PARTICLES_DESTROY_BLOCK = 2001;
    static const int PARTICLES_POTION_SPLASH = 2002;
    static const int PARTICLES_EYE_OF_ENDER_DEATH = 2003;
    static const int PARTICLES_MOBTILE_SPAWN = 2004;

	static const int ALL_PLAYERS_SLEEPING = 9800;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__LevelEvent_H__*/
