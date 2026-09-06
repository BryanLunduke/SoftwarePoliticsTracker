#include "SoundSystemSL.h"
#include <SLES/OpenSLES_Platform.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include "../../util/Mth.h"
#include "../../world/level/tile/Tile.h"
#include "../../world/phys/Vec3.h"
#include "../../client/sound/Sound.h"

#include "../log.h"

// Only one engine can be created at once. You CAN (if you really want)
// start two games at once, then it will crash without objEngine being static.
/*static*/ SLObjectItf SoundSystemSL::objEngine = 0;

typedef struct t_context {
	SLObjectItf obj;
	Mutex* mutex;
} t_context;

Mutex SoundSystemSL::toRemoveMutex;
std::vector<SLObjectItf> SoundSystemSL::toRemove;

SoundSystemSL::SoundSystemSL()
:	available(true),
	listener(NULL),
	numBuffersPlaying(0)
{
	init();
}

SoundSystemSL::~SoundSystemSL()
{
	toRemoveMutex.unlock();
	for (SoundList::iterator it = playingBuffers.begin(); it != playingBuffers.end(); ++it)
		(**it)->Destroy(*it);
	(*objOutput)->Destroy(objOutput);
	
	if (SoundSystemSL::objEngine != 0) {
	   (*SoundSystemSL::objEngine)->Destroy(SoundSystemSL::objEngine);
	   SoundSystemSL::objEngine = 0;
	}
}

void SoundSystemSL::init()
{
	SoundSystemSL::toRemove.clear();
	SoundSystemSL::toRemove.reserve(MAX_BUFFERS_PLAYING);
	toRemoveCopy.resize(MAX_BUFFERS_PLAYING);

	SLresult    res;
	const int MAX_NUMBER_INTERFACES = 2;
	SLboolean required[MAX_NUMBER_INTERFACES];
	SLInterfaceID iidArray[MAX_NUMBER_INTERFACES];

	SLEngineOption EngineOption[] = {(SLuint32)
		SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE};

	/* Create OpenSL ES (destroy first if needed)*/
	if (SoundSystemSL::objEngine != 0)
        (*SoundSystemSL::objEngine)->Destroy(SoundSystemSL::objEngine);

	res = slCreateEngine( &SoundSystemSL::objEngine, 1, EngineOption, 0, NULL, NULL);
	checkErr(res);

	/* Realizing the SL Engine in synchronous mode. */
	res = (*SoundSystemSL::objEngine)->Realize(SoundSystemSL::objEngine, SL_BOOLEAN_FALSE);
	if (checkErr(res)) {
		available = false;
		return;
	}

	(*SoundSystemSL::objEngine)->GetInterface(SoundSystemSL::objEngine, SL_IID_ENGINE, (void*)&engEngine);
	checkErr(res);

	/* Create Output Mix object to be used by player - no interfaces
	required */
	res = (*engEngine)->CreateOutputMix(engEngine, &objOutput, 0, iidArray, required);
	checkErr(res);

	/* Realizing the Output Mix object in synchronous mode. */
	res = (*objOutput)->Realize(objOutput, SL_BOOLEAN_FALSE);
	checkErr(res);
}

void SoundSystemSL::destroy() {}

void SoundSystemSL::setListenerPos( float x, float y, float z )
{
	if (!listener) {
		listenerPos = Vec3(x, y, z);
		return;
	}

	SLVec3D pos = {1000.0f * x, 1000.0f * y, 1000.0f * z};
	SLresult res = (*listener)->SetLocationCartesian(listener, &pos);
	checkErr(res);
}

void SoundSystemSL::setListenerAngle( float deg )
{
	if (!listener) return;

	SLresult res = (*listener)->SetOrientationAngles(listener, deg*1000.0f, 0, 0);
	checkErr(res);
}

void SoundSystemSL::playAt( const SoundDesc& sound, float x, float y, float z, float volume, float pitch )
{
	removeStoppedSounds();

	if (numBuffersPlaying >= MAX_BUFFERS_PLAYING)
		return;

	/* Setup the data source structure for the player */ 
	SLDataLocator_AndroidSimpleBufferQueue uri = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
	SLDataFormat_PCM mime = {
		SL_DATAFORMAT_PCM,
		sound.channels,
		sound.frameRate * 1000,
		sound.byteWidth << 3,
		sound.byteWidth << 3,
		sound.channels==1?	SL_SPEAKER_FRONT_CENTER :
		SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
		SL_BYTEORDER_LITTLEENDIAN
	};
	SLDataSource			audioSource = {&uri, &mime};
	SLDataLocator_OutputMix locator_outputmix;
	SLDataSink				audioSink;
	SLObjectItf             player; 
	SLPlayItf               playItf; 
	//SL3DLocationItf         locationItf;
	SLVolumeItf				volumeItf;

	/* Setup the data sink structure */ 
	locator_outputmix.locatorType   = SL_DATALOCATOR_OUTPUTMIX; 
	locator_outputmix.outputMix     = objOutput; 
	audioSink.pLocator              = (void *)&locator_outputmix; 
	audioSink.pFormat               = NULL; 

	/* Buffer queue-able */
	static SLboolean required[2];
	static SLInterfaceID iidArray[2];
	required[0] = SL_BOOLEAN_TRUE; 
	iidArray[0] = SL_IID_BUFFERQUEUE; 
	required[1] = SL_BOOLEAN_TRUE; 
	iidArray[1] = SL_IID_VOLUME; 

	/* Create the 3D player */ 
	SLresult res = (*engEngine)->CreateAudioPlayer(engEngine, &player, 
		&audioSource, &audioSink, 2, iidArray, required);
	//printf("SL: Created audio player\n");
	checkErr(res);

	/* Realizing the player in synchronous mode. */
	res = (*player)->Realize(player, SL_BOOLEAN_FALSE);
	//LOGI("SL: Realize audio player\n");
	checkErr(res); 

	/* Get the play and volume interfaces */ 
	res = (*player)->GetInterface(player, SL_IID_PLAY, (void*)&playItf); 
	//LOGI("SL: Get Player interface\n");
	checkErr(res);

	res = (*player)->GetInterface(player, SL_IID_VOLUME, (void*)&volumeItf); 
	//LOGI("SL: Get Player interface\n");
	checkErr(res);

    SLmillibel maxVolume;
	res = (*volumeItf)->GetMaxVolumeLevel(volumeItf, &maxVolume);
	SLmillibel mbelVolume = maxVolume - 2000 * (1-volume);//Mth::lerp(SL_MILLIBEL_MIN, maxVolume, 0.95f + 0.05f*volume);
	LOGI("min: %d, max: %d, current: %d (%f)\n", SL_MILLIBEL_MIN, maxVolume, mbelVolume, volume);
	res = (*volumeItf)->SetVolumeLevel(volumeItf, mbelVolume);
	checkErr(res);

	SLAndroidSimpleBufferQueueItf buffer1;
	res = (*player)->GetInterface(player, SL_IID_BUFFERQUEUE, &buffer1);
	checkErr(res);

	//t_context* context = new t_context(); //{ player, &toRemoveMutex };
	//context->obj = player;
	//context->mutex = &toRemoveMutex;
	res = (*buffer1)->RegisterCallback(buffer1, SoundSystemSL::removePlayer, (void*)player);
	checkErr(res);

	res = (*buffer1)->Enqueue(buffer1, sound.frames, sound.size);
	checkErr(res);

	/* Start playing the 3D source */ 
	res = (*playItf)->SetPlayState( playItf, SL_PLAYSTATE_PLAYING ); 
	//LOGI("SL: Set play state\n");
	checkErr(res);

	playingBuffers.push_back(player);
	++numBuffersPlaying;
}

bool SoundSystemSL::checkErr( SLresult res )
{
	if ( res != SL_RESULT_SUCCESS ) {
		LOGI("OpenSL error: %d\n", res);
		return true;
	}
	return false;
}

/*static*/
void SoundSystemSL::removeStoppedSounds()
{
	toRemoveMutex.lock();
	const int numBuffersToRemove = toRemove.size();
	for (int i = 0; i < numBuffersToRemove; ++i)
		toRemoveCopy[i] = toRemove[i];
	SoundSystemSL::toRemove.clear();
	toRemoveMutex.unlock();

	for (int i = 0; i < numBuffersToRemove; ++i) {
		SLObjectItf obj = toRemoveCopy[i];

		SoundList::iterator it = playingBuffers.begin();
		while (it != playingBuffers.end()) {
			if (*it == obj) {
				playingBuffers.erase(it);
				break;
			}
			++it;
		}
		(*obj)->Destroy(obj);
		--numBuffersPlaying;
	}
}

void SoundSystemSL::removePlayer( SLAndroidSimpleBufferQueueItf bq, void *context_ )
{
	//t_context* context = (t_context*) context_;
	//context->mutex->lock();
	//SoundSystemSL::toRemove.push_back( context->obj );
	//context->mutex->unlock();
	//delete context;

	SoundSystemSL::toRemoveMutex.lock();
	SoundSystemSL::toRemove.push_back( (SLObjectItf) context_ );
	SoundSystemSL::toRemoveMutex.unlock();
}
