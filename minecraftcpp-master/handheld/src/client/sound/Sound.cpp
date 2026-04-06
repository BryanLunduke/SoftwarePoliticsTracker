#include "Sound.h"

#if !defined(PRE_ANDROID23) && !defined(__APPLE__) && !defined(RPI)

#include "data/cloth1.pcm"
#include "data/cloth2.pcm"
#include "data/cloth3.pcm"
#include "data/cloth4.pcm"
#include "data/grass1.pcm"
#include "data/grass2.pcm"
#include "data/grass3.pcm"
#include "data/grass4.pcm"
#include "data/gravel1.pcm"
#include "data/gravel2.pcm"
#include "data/gravel3.pcm"
#include "data/gravel4.pcm"
#include "data/sand1.pcm"
#include "data/sand2.pcm"
#include "data/sand3.pcm"
#include "data/sand4.pcm"
#include "data/stone1.pcm"
#include "data/stone2.pcm"
#include "data/stone3.pcm"
#include "data/stone4.pcm"
#include "data/wood1.pcm"
#include "data/wood2.pcm"
#include "data/wood3.pcm"
#include "data/wood4.pcm"

#include "data/click.pcm"
#include "data/explode.pcm"
#include "data/splash.pcm"
#include "data/pop.pcm"
#include "data/pop2.pcm"
#include "data/hurt.pcm"
#include "data/door_open.pcm"
#include "data/door_close.pcm"
#include "data/glass1.pcm"
#include "data/glass2.pcm"
#include "data/glass3.pcm"

#include "data/sheep1.pcm"
#include "data/sheep2.pcm"
#include "data/sheep3.pcm"

#include "data/pig1.pcm"
#include "data/pig2.pcm"
#include "data/pig3.pcm"
#include "data/pigdeath.pcm"

//#include "data/chicken1.pcm"
#include "data/chicken2.pcm"
#include "data/chicken3.pcm"
#include "data/chickenhurt1.pcm"
#include "data/chickenhurt2.pcm"

#include "data/cow1.pcm"
#include "data/cow2.pcm"
#include "data/cow3.pcm"
#include "data/cow4.pcm"
#include "data/cowhurt1.pcm"
#include "data/cowhurt2.pcm"
#include "data/cowhurt3.pcm"

#include "data/zombie1.pcm"
#include "data/zombie2.pcm"
#include "data/zombie3.pcm"
#include "data/zombiedeath.pcm"
#include "data/zombiehurt1.pcm"
#include "data/zombiehurt2.pcm"

#include "data/zpig1.pcm"
#include "data/zpig2.pcm"
#include "data/zpig3.pcm"
#include "data/zpig4.pcm"
#include "data/zpigangry1.pcm"
#include "data/zpigangry2.pcm"
#include "data/zpigangry3.pcm"
#include "data/zpigangry4.pcm"
#include "data/zpigdeath.pcm"
#include "data/zpighurt1.pcm"
#include "data/zpighurt2.pcm"

#include "data/bow.pcm"
#include "data/bowhit1.pcm"
#include "data/bowhit2.pcm"
#include "data/bowhit3.pcm"
#include "data/bowhit4.pcm"
#include "data/fallbig1.pcm"
#include "data/fallbig2.pcm"
#include "data/fallsmall.pcm"
#include "data/skeleton1.pcm"
#include "data/skeleton2.pcm"
#include "data/skeleton3.pcm"
#include "data/skeletondeath.pcm"
#include "data/skeletonhurt1.pcm"
#include "data/skeletonhurt2.pcm"
#include "data/skeletonhurt3.pcm"
#include "data/skeletonhurt4.pcm"
#include "data/spider1.pcm"
#include "data/spider2.pcm"
#include "data/spider3.pcm"
#include "data/spider4.pcm"
#include "data/spiderdeath.pcm"

#include "data/creeper1.pcm"
#include "data/creeper2.pcm"
#include "data/creeper3.pcm"
#include "data/creeper4.pcm"
#include "data/creeperdeath.pcm"
#include "data/eat1.pcm"
#include "data/eat2.pcm"
#include "data/eat3.pcm"
#include "data/fuse.pcm"

SoundDesc SA_cloth1((char*)PCM_cloth1);
SoundDesc SA_cloth2((char*)PCM_cloth2);
SoundDesc SA_cloth3((char*)PCM_cloth3);
SoundDesc SA_cloth4((char*)PCM_cloth4);
SoundDesc SA_grass1((char*)PCM_grass1);
SoundDesc SA_grass2((char*)PCM_grass2);
SoundDesc SA_grass3((char*)PCM_grass3);
SoundDesc SA_grass4((char*)PCM_grass4);
SoundDesc SA_gravel1((char*)PCM_gravel1);
SoundDesc SA_gravel2((char*)PCM_gravel2);
SoundDesc SA_gravel3((char*)PCM_gravel3);
SoundDesc SA_gravel4((char*)PCM_gravel4);
SoundDesc SA_sand1((char*)PCM_sand1);
SoundDesc SA_sand2((char*)PCM_sand2);
SoundDesc SA_sand3((char*)PCM_sand3);
SoundDesc SA_sand4((char*)PCM_sand4);
SoundDesc SA_stone1((char*)PCM_stone1);
SoundDesc SA_stone2((char*)PCM_stone2);
SoundDesc SA_stone3((char*)PCM_stone3);
SoundDesc SA_stone4((char*)PCM_stone4);
SoundDesc SA_wood1((char*)PCM_wood1);
SoundDesc SA_wood2((char*)PCM_wood2);
SoundDesc SA_wood3((char*)PCM_wood3);
SoundDesc SA_wood4((char*)PCM_wood4);

SoundDesc SA_click((char*)PCM_click);
SoundDesc SA_explode((char*)PCM_explode);
SoundDesc SA_splash((char*)PCM_splash);

SoundDesc SA_door_open((char*)PCM_door_open);
SoundDesc SA_door_close((char*)PCM_door_close);
SoundDesc SA_pop((char*)PCM_pop);
SoundDesc SA_hurt((char*)PCM_hurt);
SoundDesc SA_glass1((char*)PCM_glass1);
SoundDesc SA_glass2((char*)PCM_glass2);
SoundDesc SA_glass3((char*)PCM_glass3);

SoundDesc SA_sheep1((char*)PCM_sheep1);
SoundDesc SA_sheep2((char*)PCM_sheep2);
SoundDesc SA_sheep3((char*)PCM_sheep3);
SoundDesc SA_pig1((char*)PCM_pig1);
SoundDesc SA_pig2((char*)PCM_pig2);
SoundDesc SA_pig3((char*)PCM_pig3);
SoundDesc SA_pigdeath((char*)PCM_pigdeath);

//SoundDesc SA_chicken1((char*)PCM_chicken1);
SoundDesc SA_chicken2((char*)PCM_chicken2);
SoundDesc SA_chicken3((char*)PCM_chicken3);
SoundDesc SA_chickenhurt1((char*)PCM_chickenhurt1);
SoundDesc SA_chickenhurt2((char*)PCM_chickenhurt2);
SoundDesc SA_cow1((char*)PCM_cow1);
SoundDesc SA_cow2((char*)PCM_cow2);
SoundDesc SA_cow3((char*)PCM_cow3);
SoundDesc SA_cow4((char*)PCM_cow4);
SoundDesc SA_cowhurt1((char*)PCM_cowhurt1);
SoundDesc SA_cowhurt2((char*)PCM_cowhurt2);
SoundDesc SA_cowhurt3((char*)PCM_cowhurt3);

SoundDesc SA_zombie1((char*)PCM_zombie1);
SoundDesc SA_zombie2((char*)PCM_zombie2);
SoundDesc SA_zombie3((char*)PCM_zombie3);

SoundDesc SA_zpig1((char*)PCM_zpig1);
SoundDesc SA_zpig2((char*)PCM_zpig2);
SoundDesc SA_zpig3((char*)PCM_zpig3);
SoundDesc SA_zpig4((char*)PCM_zpig4);
SoundDesc SA_zpigangry1((char*)PCM_zpigangry1);
SoundDesc SA_zpigangry2((char*)PCM_zpigangry2);
SoundDesc SA_zpigangry3((char*)PCM_zpigangry3);
SoundDesc SA_zpigangry4((char*)PCM_zpigangry4);
SoundDesc SA_zpigdeath((char*)PCM_zpigdeath);
SoundDesc SA_zpighurt1((char*)PCM_zpighurt1);
SoundDesc SA_zpighurt2((char*)PCM_zpighurt2);

SoundDesc SA_zombiedeath((char*)PCM_zombiedeath);
SoundDesc SA_zombiehurt1((char*)PCM_zombiehurt1);
SoundDesc SA_zombiehurt2((char*)PCM_zombiehurt2);

SoundDesc SA_bow((char*)PCM_bow);
SoundDesc SA_bowhit1((char*)PCM_bowhit1);
SoundDesc SA_bowhit2((char*)PCM_bowhit2);
SoundDesc SA_bowhit3((char*)PCM_bowhit3);
SoundDesc SA_bowhit4((char*)PCM_bowhit4);
SoundDesc SA_fallbig1((char*)PCM_fallbig1);
SoundDesc SA_fallbig2((char*)PCM_fallbig2);
SoundDesc SA_fallsmall((char*)PCM_fallsmall);
SoundDesc SA_skeleton1((char*)PCM_skeleton1);
SoundDesc SA_skeleton2((char*)PCM_skeleton2);
SoundDesc SA_skeleton3((char*)PCM_skeleton3);
SoundDesc SA_skeletondeath((char*)PCM_skeletondeath);
SoundDesc SA_skeletonhurt1((char*)PCM_skeletonhurt1);
SoundDesc SA_skeletonhurt2((char*)PCM_skeletonhurt2);
SoundDesc SA_skeletonhurt3((char*)PCM_skeletonhurt3);
SoundDesc SA_skeletonhurt4((char*)PCM_skeletonhurt4);
SoundDesc SA_spider1((char*)PCM_spider1);
SoundDesc SA_spider2((char*)PCM_spider2);
SoundDesc SA_spider3((char*)PCM_spider3);
SoundDesc SA_spider4((char*)PCM_spider4);
SoundDesc SA_spiderdeath((char*)PCM_spiderdeath);

SoundDesc SA_creeper1((char*)PCM_creeper1);
SoundDesc SA_creeper2((char*)PCM_creeper2);
SoundDesc SA_creeper3((char*)PCM_creeper3);
SoundDesc SA_creeper4((char*)PCM_creeper4);
SoundDesc SA_creeperdeath((char*)PCM_creeperdeath);
SoundDesc SA_eat1((char*)PCM_eat1);
SoundDesc SA_eat2((char*)PCM_eat2);
SoundDesc SA_eat3((char*)PCM_eat3);
SoundDesc SA_fuse((char*)PCM_fuse);

#endif /*!PRE_ANDROID23 && !__APPLE__*/
