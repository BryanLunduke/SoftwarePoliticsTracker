#include "SoundEngine.h"
#include "../Options.h"
#include "../Minecraft.h"
#include "../../world/entity/Mob.h"

/*
 
 File: MyOpenALSupport.c
 Abstract: OpenAL-related support functions
 Version: 1.4
 
 Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
 Inc. ("Apple") in consideration of your agreement to the following
 terms, and your use, installation, modification or redistribution of
 this Apple software constitutes acceptance of these terms.  If you do
 not agree with these terms, please do not use, install, modify or
 redistribute this Apple software.
 
 In consideration of your agreement to abide by the following terms, and
 subject to these terms, Apple grants you a personal, non-exclusive
 license, under Apple's copyrights in this original Apple software (the
 "Apple Software"), to use, reproduce, modify and redistribute the Apple
 Software, with or without modifications, in source and/or binary forms;
 provided that if you redistribute the Apple Software in its entirety and
 without modifications, you must retain this notice and the following
 text and disclaimers in all such redistributions of the Apple Software.
 Neither the name, trademarks, service marks or logos of Apple Inc. may
 be used to endorse or promote products derived from the Apple Software
 without specific prior written permission from Apple.  Except as
 expressly stated in this notice, no other rights or licenses, express or
 implied, are granted by Apple herein, including but not limited to any
 patent rights that may be infringed by your derivative works or by other
 works in which the Apple Software may be incorporated.
 
 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 
 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 
 Copyright (C) 2009 Apple Inc. All Rights Reserved.
 
 
 */

#import <OpenAL/al.h>
#import <OpenAL/alc.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioToolbox/ExtendedAudioFile.h>


//ALvoid  alBufferDataStaticProc(const ALint bid, ALenum format, ALvoid* data, ALsizei size, ALsizei freq)
//{
//	static	alBufferDataStaticProcPtr	proc = NULL;
//    
//    if (proc == NULL) {
//        proc = (alBufferDataStaticProcPtr) alcGetProcAddress(NULL, (const ALCchar*) "alBufferDataStatic");
//    }
//    
//    if (proc)
//        proc(bid, format, data, size, freq);
//
//    return;
//}

void* MyGetOpenALAudioData(CFURLRef inFileURL, ALsizei *outDataSize, ALenum *outDataFormat, ALsizei*	outSampleRate)
{
	OSStatus						err = noErr;	
	SInt64							theFileLengthInFrames = 0;
	AudioStreamBasicDescription		theFileFormat;
	UInt32							thePropertySize = sizeof(theFileFormat);
	ExtAudioFileRef					extRef = NULL;
	void*							theData = NULL;
	AudioStreamBasicDescription		theOutputFormat;
    
    do {
	// Open a file with ExtAudioFileOpen()
	err = ExtAudioFileOpenURL(inFileURL, &extRef);
	if(err) { printf("MyGetOpenALAudioData: ExtAudioFileOpenURL FAILED, Error = %ld\n", err); break; }
	
	// Get the audio data format
	err = ExtAudioFileGetProperty(extRef, kExtAudioFileProperty_FileDataFormat, &thePropertySize, &theFileFormat);
	if(err) { printf("MyGetOpenALAudioData: ExtAudioFileGetProperty(kExtAudioFileProperty_FileDataFormat) FAILED, Error = %ld\n", err); break; }
	if (theFileFormat.mChannelsPerFrame > 2)  { printf("MyGetOpenALAudioData - Unsupported Format, channel count is greater than stereo\n"); break;}
    
	// Set the client format to 16 bit signed integer (native-endian) data
	// Maintain the channel count and sample rate of the original source format
	theOutputFormat.mSampleRate = theFileFormat.mSampleRate;
	theOutputFormat.mChannelsPerFrame = theFileFormat.mChannelsPerFrame;
    
	theOutputFormat.mFormatID = kAudioFormatLinearPCM;
	theOutputFormat.mBytesPerPacket = 2 * theOutputFormat.mChannelsPerFrame;
	theOutputFormat.mFramesPerPacket = 1;
	theOutputFormat.mBytesPerFrame = 2 * theOutputFormat.mChannelsPerFrame;
	theOutputFormat.mBitsPerChannel = 16;
	theOutputFormat.mFormatFlags = kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked | kAudioFormatFlagIsSignedInteger;
	
	// Set the desired client (output) data format
	err = ExtAudioFileSetProperty(extRef, kExtAudioFileProperty_ClientDataFormat, sizeof(theOutputFormat), &theOutputFormat);
	if(err) { printf("MyGetOpenALAudioData: ExtAudioFileSetProperty(kExtAudioFileProperty_ClientDataFormat) FAILED, Error = %ld\n", err); break; }
	
	// Get the total frame count
	thePropertySize = sizeof(theFileLengthInFrames);
	err = ExtAudioFileGetProperty(extRef, kExtAudioFileProperty_FileLengthFrames, &thePropertySize, &theFileLengthInFrames);
	if(err) { printf("MyGetOpenALAudioData: ExtAudioFileGetProperty(kExtAudioFileProperty_FileLengthFrames) FAILED, Error = %ld\n", err); break; }
	
	// Read all the data into memory
	UInt32		dataSize = theFileLengthInFrames * theOutputFormat.mBytesPerFrame;
	theData = (void*)new char[dataSize];
	if (theData)
	{
		AudioBufferList		theDataBuffer;
		theDataBuffer.mNumberBuffers = 1;
		theDataBuffer.mBuffers[0].mDataByteSize = dataSize;
		theDataBuffer.mBuffers[0].mNumberChannels = theOutputFormat.mChannelsPerFrame;
		theDataBuffer.mBuffers[0].mData = theData;
		
		// Read the data into an AudioBufferList
		err = ExtAudioFileRead(extRef, (UInt32*)&theFileLengthInFrames, &theDataBuffer);
		if(err == noErr)
		{
			// success
			*outDataSize = (ALsizei)dataSize;
			*outDataFormat = (theOutputFormat.mChannelsPerFrame > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
			*outSampleRate = (ALsizei)theOutputFormat.mSampleRate;
		}
		else 
		{ 
			// failure
			free (theData);
			theData = NULL; // make sure to return NULL
			printf("MyGetOpenALAudioData: ExtAudioFileRead FAILED, Error = %ld\n", err); break;
		}	
	}
    } while(0);
	
	// Dispose the ExtAudioFileRef, it is no longer needed
	if (extRef) ExtAudioFileDispose(extRef);
	return theData;
}



SoundEngine::SoundEngine( float maxDistance )
:	idCounter(0),
	mc(0),
	_x(0),
	_y(0),
	_z(0),
	_yRot(0),
	_invMaxDistance(1.0f / maxDistance)
{
}

SoundEngine::~SoundEngine()
{
}

SoundDesc SoundEngine::_pp(const std::string& fn) {
    NSString* filename = [[NSString alloc] initWithUTF8String:fn.c_str()];
    NSString* path = [[NSBundle mainBundle] pathForResource:filename ofType:@"m4a"];
    [filename release];
    if (!path)
        return SoundDesc();

    CFURLRef fileURL = (CFURLRef)[[NSURL fileURLWithPath:path] retain];
    if (fileURL) {
        static Stopwatch sw;
        sw.start();

        ALsizei size, freq;
        ALenum format;
        void* data = MyGetOpenALAudioData(fileURL, &size, &format, &freq);

        
        CFRelease(fileURL);
        sw.stop();
        sw.printEvery(1, "load sound " + fn + " :: ");
        int channels = (format == AL_FORMAT_STEREO16 || format == AL_FORMAT_STEREO8)? 2 : 1;
        int bytewidth= (format == AL_FORMAT_STEREO16 || format == AL_FORMAT_MONO16)?  2 : 1;

        return SoundDesc((char*)data, size, channels, bytewidth, freq);
    }
    return SoundDesc();
}

void SoundEngine::init( Minecraft* mc, Options* options )
{
	this->mc = mc;
	this->options = options;

	if (/*!loaded && */(options == NULL || (options->sound != 0 || options->music != 0))) {
		loadLibrary();
	}
    
    sounds.add(	"damage.fallbig", _pp("fallbig1"));
    sounds.add(	"damage.fallbig", _pp("fallbig2"));
    sounds.add(	"damage.fallsmall", _pp("fallsmall"));
    sounds.add(	"random.explode", _pp("explode"));
    sounds.add(	"random.splash", _pp("splash"));
    sounds.add(	"random.hurt", _pp("hurt"));
    sounds.add(	"random.pop", _pp("pop"));
    sounds.add(	"random.click", _pp("click"));

    sounds.add(	"random.door_open", _pp("door_open"));
    sounds.add(	"random.door_close", _pp("door_close"));

    sounds.add(	"random.bow", _pp("bow"));
    sounds.add(	"random.bowhit", _pp("bowhit1"));
    sounds.add(	"random.bowhit", _pp("bowhit2"));
    sounds.add(	"random.bowhit", _pp("bowhit3"));
    sounds.add(	"random.bowhit", _pp("bowhit4"));

    sounds.add(	"random.glass", _pp("glass1"));
    sounds.add(	"random.glass", _pp("glass2"));
    sounds.add(	"random.glass", _pp("glass3"));

    sounds.add( "random.eat",  _pp("eat1"));
    sounds.add( "random.eat",  _pp("eat2"));
    sounds.add( "random.eat",  _pp("eat3"));
    sounds.add( "random.fuse", _pp("fuse"));

    sounds.add(	"step.cloth", _pp("cloth1"));
    sounds.add(	"step.cloth", _pp("cloth2"));
    sounds.add(	"step.cloth", _pp("cloth3"));
    sounds.add(	"step.cloth", _pp("cloth4"));

    sounds.add(	"step.grass", _pp("grass1"));
    sounds.add(	"step.grass", _pp("grass2"));
    sounds.add(	"step.grass", _pp("grass3"));
    sounds.add(	"step.grass", _pp("grass4"));

			//new SoundId(R.raw.gravel1, _pp("step.gravel"),
    sounds.add(	"step.gravel", _pp("gravel2"));
    sounds.add(	"step.gravel", _pp("gravel3"));
    sounds.add(	"step.gravel", _pp("gravel4"));

    sounds.add(	"step.sand", _pp("sand1"));
    sounds.add(	"step.sand", _pp("sand2"));
    sounds.add(	"step.sand", _pp("sand3"));
    sounds.add(	"step.sand", _pp("sand4"));

    sounds.add(	"step.stone", _pp("stone1"));
    sounds.add(	"step.stone", _pp("stone2"));
    sounds.add(	"step.stone", _pp("stone3"));
    sounds.add(	"step.stone", _pp("stone4"));

    sounds.add(	"step.wood", _pp("wood1"));
    sounds.add(	"step.wood", _pp("wood2"));
    sounds.add(	"step.wood", _pp("wood3"));
    sounds.add(	"step.wood", _pp("wood4"));

    sounds.add(	"mob.sheep", _pp("sheep1"));
    sounds.add(	"mob.sheep", _pp("sheep2"));
    sounds.add(	"mob.sheep", _pp("sheep3"));

    sounds.add(	"mob.pig", _pp("pig1"));
    sounds.add(	"mob.pig", _pp("pig2"));
    sounds.add(	"mob.pig", _pp("pig3"));
    sounds.add(	"mob.pigdeath", _pp("pigdeath"));

    //sounds.add( "mob.chicken", _pp("chicken1"));
    sounds.add( "mob.chicken", _pp("chicken2"));
    sounds.add( "mob.chicken", _pp("chicken3"));
    sounds.add( "mob.chickenhurt", _pp("chickenhurt1"));
    sounds.add( "mob.chickenhurt", _pp("chickenhurt2"));

    sounds.add( "mob.cow", _pp("cow1"));
    sounds.add( "mob.cow", _pp("cow2"));
    sounds.add( "mob.cow", _pp("cow3"));
    sounds.add( "mob.cow", _pp("cow4"));
    sounds.add( "mob.cowhurt", _pp("cowhurt1"));
    sounds.add( "mob.cowhurt", _pp("cowhurt2"));
    sounds.add( "mob.cowhurt", _pp("cowhurt3"));

    sounds.add(	"mob.zombie", _pp("zombie1"));
    sounds.add(	"mob.zombie", _pp("zombie2"));
    sounds.add(	"mob.zombie", _pp("zombie3"));
    sounds.add(	"mob.zombiedeath", _pp("zombiedeath"));
    sounds.add(	"mob.zombiehurt", _pp("zombiehurt1"));
    sounds.add(	"mob.zombiehurt", _pp("zombiehurt2"));

    sounds.add(	"mob.skeleton", _pp("skeleton1"));
    sounds.add(	"mob.skeleton", _pp("skeleton2"));
    sounds.add(	"mob.skeleton", _pp("skeleton3"));
    sounds.add(	"mob.skeletonhurt", _pp("skeletonhurt1"));
    sounds.add(	"mob.skeletonhurt", _pp("skeletonhurt2"));
    sounds.add(	"mob.skeletonhurt", _pp("skeletonhurt3"));
    sounds.add(	"mob.skeletonhurt", _pp("skeletonhurt4"));

    sounds.add(	"mob.spider", _pp("spider1"));
    sounds.add(	"mob.spider", _pp("spider2"));
    sounds.add(	"mob.spider", _pp("spider3"));
    sounds.add(	"mob.spiderdeath", _pp("spiderdeath"));

    sounds.add( "mob.creeper",      _pp("creeper1"));
    sounds.add( "mob.creeper",      _pp("creeper2"));
    sounds.add( "mob.creeper",      _pp("creeper3"));
    sounds.add( "mob.creeper",      _pp("creeper4"));
    sounds.add( "mob.creeperdeath", _pp("creeperdeath"));

    sounds.add( "mob.zombiepig.zpig",       _pp("zpig1"));
    sounds.add( "mob.zombiepig.zpig",       _pp("zpig2"));
    sounds.add( "mob.zombiepig.zpig",       _pp("zpig3"));
    sounds.add( "mob.zombiepig.zpig",       _pp("zpig4"));
    sounds.add( "mob.zombiepig.zpigangry",  _pp("zpigangry1"));
    sounds.add( "mob.zombiepig.zpigangry",  _pp("zpigangry2"));
    sounds.add( "mob.zombiepig.zpigangry",  _pp("zpigangry3"));
    sounds.add( "mob.zombiepig.zpigangry",  _pp("zpigangry4"));
    sounds.add( "mob.zombiepig.zpigdeath",  _pp("zpigdeath"));
    sounds.add( "mob.zombiepig.zpighurt",   _pp("zpighurt1"));
    sounds.add( "mob.zombiepig.zpighurt",   _pp("zpighurt2"));
}

void SoundEngine::enable( bool status )
{
	soundSystem.enable(status);
}

void SoundEngine::updateOptions()
{

}

void SoundEngine::destroy()
{
	//if (loaded) soundSystem.cleanup();
}

void SoundEngine::update( Mob* player, float a )
{
	if (/*!loaded || */options->sound == 0) return;
	if (player == NULL) return;

	_x = player->xo + (player->x - player->xo) * a;
	_y = player->yo + (player->y - player->yo) * a;
	_z = player->zo + (player->z - player->zo) * a;
	_yRot = player->yRotO + (player->yRot - player->yRotO) * a;

	soundSystem.setListenerAngle(_yRot);
	//soundSystem.setListenerPos(_x, _y, _z); //@note: not used, since we translate all sounds to Player space
}

float SoundEngine::_getVolumeMult( float x, float y, float z )
{
	const float dx = x - _x;
	const float dy = y - _y;
	const float dz = z - _z;
	const float dist = Mth::sqrt(dx*dx + dy*dy + dz*dz);
	const float out =  Mth::clamp(1.1f - dist*_invMaxDistance, -1.0f, 1.0f);
	return out;
}

void SoundEngine::play(const std::string& name, float x, float y, float z, float volume, float pitch) {
	if ((volume *= options->sound) <= 0) return;

	volume = Mth::clamp(volume, 0.0f, 1.0f);

	SoundDesc sound;
	if (sounds.get(name, sound)) {
		soundSystem.playAt(sound, x-_x, y-_y, z-_z, volume, pitch);
	}
}

void SoundEngine::playUI(const std::string& name, float volume, float pitch) {
	if ((volume *= options->sound) <= 0) return;

	volume = Mth::clamp(volume, 0.0f, 1.0f);
	if (/*!loaded || */options->sound == 0 || volume <= 0) return;

	SoundDesc sound;
	if (sounds.get(name, sound)) {
		soundSystem.playAt(sound, 0, 0, 0, volume, pitch);
	}
}

