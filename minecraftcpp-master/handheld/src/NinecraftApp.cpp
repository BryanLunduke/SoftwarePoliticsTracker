#include "NinecraftApp.h"
//#include <EGL/egl.h>

#ifdef RPI
//#define NO_STORAGE
#endif

#include <errno.h>

#include "platform/input/Mouse.h"
#include "platform/input/Multitouch.h"

#include "world/item/Item.h"
#include "world/level/Level.h"
#include "world/level/biome/Biome.h"
#include "world/level/material/Material.h"
#include "world/entity/MobCategory.h"
//#include "world/level/storage/FolderMethods.h"
#ifndef STANDALONE_SERVER
#include "client/gui/screens/StartMenuScreen.h"
#endif
#include "client/player/LocalPlayer.h"
#ifndef STANDALONE_SERVER
#include "client/renderer/gles.h"
#include "client/renderer/Chunk.h"
#include "client/renderer/LevelRenderer.h"
#include "client/renderer/Tesselator.h"
#endif
// sorry for raknet dependency, but I'm too lazy to find another getTime method
#include "raknet/GetTime.h"
#include "network/RakNetInstance.h"
#include "network/ClientSideNetworkHandler.h"
#include "client/gui/screens/ProgressScreen.h"

//#include "world/entity/player/Inventory2.h"

#if !defined(DEMO_MODE) && !defined(APPLE_DEMO_PROMOTION) && !defined(NO_STORAGE)
	#include "world/level/storage/ExternalFileLevelStorageSource.h"
#else
	#include "world/level/storage/MemoryLevelStorageSource.h"
#endif
#ifndef STANDALONE_SERVER
#include "client/renderer/Textures.h"
#include "client/renderer/entity/ItemRenderer.h"
#endif
#include "world/item/crafting/Recipes.h"
#include "world/level/tile/entity/TileEntity.h"
#ifndef STANDALONE_SERVER
#include "client/renderer/EntityTileRenderer.h"
#endif

bool NinecraftApp::_hasInitedStatics = false;

NinecraftApp::NinecraftApp()
:   _verbose(true),
	_lastTickMs(0),
	_frames(0)
{
    #if defined(ANDROID) || defined(__APPLE__) || defined(RPI)
        signal(SIGPIPE, SIG_IGN);
    #endif
}

NinecraftApp::~NinecraftApp()
{
	teardown();
}

void NinecraftApp::init()
{
	// Global initialization goes here
	Mth::initMth();

	//#ifdef DEMO_MODE
	//writeDemoFile();
	//#endif

	// If we're running Android, only initialize
	// the first time class is instanced
	#ifdef ANDROID
	if (!_hasInitedStatics) {
		_hasInitedStatics = true;
	#endif
		Material::initMaterials();
		MobCategory::initMobCategories();
		Tile::initTiles();
		Item::initItems();
		Biome::initBiomes();
		TileEntity::initTileEntities();

	#ifdef ANDROID
	}
	#endif

#ifndef STANDALONE_SERVER
	initGLStates();
	Tesselator::instance.init();
	I18n::loadLanguage(platform(), "en_US");
#endif

	Minecraft::init();

#if !defined(DEMO_MODE) && !defined(APPLE_DEMO_PROMOTION) && !defined(NO_STORAGE)
	storageSource = new ExternalFileLevelStorageSource(externalStoragePath, externalCacheStoragePath);
#else
	storageSource = new MemoryLevelStorageSource();
#endif
	_running = false;

#ifndef STANDALONE_SERVER
	LOGI("This: %p\n", this);
	screenChooser.setScreen(SCREEN_STARTMENU);
#else
	user->name = "Server";
	hostMultiplayer();
#endif
}

void NinecraftApp::teardown()
{
	// Note: Don't tear down statics if we run on Android
	// (we might change this in the future)
#ifndef ANDROID
	Biome::teardownBiomes();
	Item ::teardownItems();
	Tile ::teardownTiles();
	Material::teardownMaterials();
	Recipes ::teardownRecipes();
	TileEntity::teardownTileEntities();
#endif
#ifdef WIN32
	ItemRenderer::teardown_static();
	if (EntityTileRenderer::instance != NULL) {
		delete EntityTileRenderer::instance;
		EntityTileRenderer::instance = NULL;
	}
	TileEntityRenderDispatcher::destroy();
#endif
}

void NinecraftApp::update()
{
	++_frames;

	// Generate Multitouch active pointer list
	Multitouch::commit();

#ifndef ANDROID_PUBLISH
	//testCreationAndDestruction();
	//testJoiningAndDestruction();
#endif /*ANDROID_PUBLISH*/

	Minecraft::update();

	swapBuffers();
	Mouse::reset2();

    // Restart the server if (our modded) RakNet reports an error
    if (level && raknetInstance->isProbablyBroken() && raknetInstance->isServer()) {
        restartServer();
    }

#ifndef WIN32
	updateStats();
#endif
}

void NinecraftApp::updateStats()
{
#ifndef STANDALONE_SERVER
	if (Options::debugGl)
		LOGI("--------------------------------------------\n");

	//*
	int now = getTimeMs();
	//int since = now - _lastTickMs;

	if (now >= lastTime + 1000)
	{
		if (player) {
			LOGI("%d fps   \t%3d chunk updates.   (%.2f, %.2f, %.2f)\n",
				_frames, Chunk::updates, player->x, player->y, player->z);
			Chunk::resetUpdates();

			//static int _n = 0;
			//if (++_n % 5 == -1) { // @note: -1
			//	static char filename[256];
			//	sprintf(filename, "%s/games/com.mojang/img_%.4d.jpg", externalStoragePath.c_str(), _n/5);
			//	_context.platform->saveScreenshot(filename, width, height);
			//}

			LOGI("%s", levelRenderer->gatherStats1().c_str());
			//printf("Texture swaps (this frame): %d\n", Textures::textureChanges);
		} else {
			LOGI("%d fps\n", _frames);
		}

		//const int* pointerIds;
		//int pointerCount = Multitouch::getActivePointerIds(&pointerIds);
		//if (pointerCount) {
		//	std::string s = "Pointers (";
		//	s += (char)(48 + pointerCount);
		//	s += ": ";
		//	for (int i = 0; i < pointerCount; ++i) {
		//		s += (char)(48 + pointerIds[i]);
		//		s += ", ";
		//	}
		//	LOGI("%s\n", s.c_str());
		//}

		lastTime = now;
		_frames = 0;
#ifdef GLDEBUG
		while (1) {
			int error = glGetError();
			if (error == GL_NO_ERROR) break;

			LOGI("#################### GL-ERROR: %d\t#####################\n", error);
			LOGI("#################### GL-ERROR: %d\t#####################\n", error);
			LOGI("#################### GL-ERROR: %d\t#####################\n", error);
		}
#endif
	}
	Textures::textureChanges = 0;
	/**/
#endif /* STANDALONE_SERVER */
}

void NinecraftApp::initGLStates()
{
#ifndef STANDALONE_SERVER
	//glShadeModel2(GL_SMOOTH);
	//glClearDepthf(1.0f);
	glEnable2(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthRangef(0, 1);
	glEnable2(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable2(GL_TEXTURE_2D);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	// Both updates isPowerVR flag in java and returns if the graphics chip is PowerVR SGX or not
	_powerVr = platform()->isPowerVR();
#ifdef __APPLE__
    _isSuperFast = platform()->isSuperFast();
#endif
	//glLineWidth(4);
#endif /* STANDALONE_SERVER */
}

void NinecraftApp::restartServer() {
    if (!level) return;

    for (int i = level->players.size()-1; i >= 0; --i) {
        Player* p = level->players[i];
        if (p != player)
            level->removeEntity(p);
    }

	raknetInstance->resetIsBroken();
#ifndef STANDALONE_SERVER
    gui.addMessage("This server has restarted!");
#endif
    hostMultiplayer();
    if (netCallback) netCallback->levelGenerated(level);
}

bool NinecraftApp::handleBack(bool isDown)
{
	if (isGeneratingLevel)
	{
		return true;
	}
	if (level)
	{
		if (!isDown)
		{
			if (screen)
			{
				if (!screen->handleBackEvent(isDown))
				{
					if (player->containerMenu) player->closeContainer();
					setScreen(NULL);
				}
				return true;
			} else {
				pauseGame(true);
			}
			//leaveGame();
			return false;
		}
		return true;
	}
	else if (screen)
	{
		return screen->handleBackEvent(isDown);
	}
	return false;
}

void NinecraftApp::onGraphicsReset()
{
#ifndef STANDALONE_SERVER
	initGLStates();
	Tesselator::instance.init();

	Minecraft::onGraphicsReset();
#endif
}

#ifndef ANDROID_PUBLISH

static int _state = -1;
static int _stateTicksLeft = 0;
void NinecraftApp::testCreationAndDestruction()
{
	if (_state == -1) {
		_stateTicksLeft = 100;
		_state = 0;
	}
	if (_state == 0) {
		if (--_stateTicksLeft <= 0)
			_state = 1;
	}
	else if (_state == 1) {
		getLevelSource()->deleteLevel("perf");
		int seed = getEpochTimeS();
		LOGI(">seed %d\n", seed);
		selectLevel("perf", "perf", LevelSettings(seed, GameType::Creative));
		hostMultiplayer();
#ifndef STANDALONE_SERVER
		setScreen(new ProgressScreen());
#endif
		_state = 2;
		_stateTicksLeft = 1000;//25000;//00;
	}
	else if (_state == 2) {
		if (isLevelGenerated()) {
			if (--_stateTicksLeft <= 0) {
				_state = 3;
			}
		}
	} else if (_state == 3) {
		leaveGame();
		_state = 1;
	}
}


void NinecraftApp::testJoiningAndDestruction()
{
	if (_state == -1) {
		//LightUpdate sz[2] = {	LightUpdate(LightLayer::Block, 0, 0, 0, 1, 1, 1), 
		//						LightUpdate(LightLayer::Sky, 0, 0, 0, 1, 1, 1) };
		//LOGI("size of lightupdate: %lu == %d\n", sizeof(LightUpdate), (const char*)&sz[1] - (const char*)&sz[0]);
		_stateTicksLeft = 100;
		_state = 0;
	}
	if (_state == 0) {
		if (--_stateTicksLeft <= 0) {
			raknetInstance->clearServerList();
			locateMultiplayer();
			_state = 1;
		}
	}
	else if (_state == 1) {
		if (!raknetInstance->getServerList().empty()) {
			PingedCompatibleServer s = raknetInstance->getServerList().at(0);
			if (s.name.GetLength() > 0) {
				joinMultiplayer(s);
#ifndef STANDALONE_SERVER
				setScreen(new ProgressScreen());
#endif
				_state = 2;
				_stateTicksLeft = 80;//1000;
			}
		}
	}
	else if (_state == 2) {
		if (isLevelGenerated()) {
			if (--_stateTicksLeft <= 0) {
				_state = 3;
			}
		}
	} else if (_state == 3) {
		leaveGame();
		_stateTicksLeft = 50;
		_state = 0;
	}
}

#endif /*ANDROID_PUBLISH*/

/*
void NinecraftApp::writeDemoFile() {
	std::string path = externalStoragePath + "/games";

	if (createFolderIfNotExists(path.c_str())) {
	path += "/com.mojang";
	if (createFolderIfNotExists(path.c_str())) {
	path += "/minecraftpe";
	if (createFolderIfNotExists(path.c_str())) {
		path += "/played_demo";
		FILE* fp = fopen(path.c_str(), "w");
		if (fp) fclose(fp);
	}}}
}

bool NinecraftApp::hasPlayedDemo() {
	std::string filename = externalStoragePath + "/games/com.mojang/minecraftpe/played_demo";
	FILE* fp = fopen(filename.c_str(), "r");
	if (!fp) return false;
	fclose(fp);
	return true;
}
*/
