#ifndef NET_MINECRAFT_CLIENT__Options_H__
#define NET_MINECRAFT_CLIENT__Options_H__

//package net.minecraft.client;

//#include "locale/Language.h"

#include <string>
#include <cstdio>
#include "KeyMapping.h"
#include "../platform/input/Keyboard.h"
#include "../util/StringUtils.h"
#include "OptionsFile.h"

class Minecraft;
typedef std::vector<std::string> StringVector;

class Options
{
public:
    class Option
	{
        const bool _isProgress;
        const bool _isBoolean;
        const std::string _captionId;
		const int _ordinal;

	public:
		static const Option MUSIC;
		static const Option SOUND;
		static const Option INVERT_MOUSE;
		static const Option SENSITIVITY;
		static const Option RENDER_DISTANCE;
		static const Option VIEW_BOBBING;
		static const Option ANAGLYPH;
		static const Option LIMIT_FRAMERATE;
		static const Option DIFFICULTY;
		static const Option GRAPHICS;
		static const Option AMBIENT_OCCLUSION;
		static const Option GUI_SCALE;
        
		static const Option THIRD_PERSON;
		static const Option HIDE_GUI;
		static const Option SERVER_VISIBLE;
		static const Option LEFT_HANDED;
		static const Option USE_TOUCHSCREEN;
		static const Option USE_TOUCH_JOYPAD;
		static const Option DESTROY_VIBRATION;

		static const Option PIXELS_PER_MILLIMETER;

		/*
        static Option* getItem(int id) {
            for (Option item : Option.values()) {
                if (item.getId() == id) {
                    return item;
                }
            }
            return NULL;
        }
		*/

        Option(int ordinal, const std::string& captionId, bool hasProgress, bool isBoolean)
		:	_captionId(captionId),
			_isProgress(hasProgress),
			_isBoolean(isBoolean),
			_ordinal(ordinal)
		{}

        bool isProgress() const {
            return _isProgress;
        }

        bool isBoolean() const {
            return _isBoolean;
        }

		bool isInt() const {
			return (!_isBoolean && !_isProgress);
		}

        int getId() {
            return _ordinal;
        }

        std::string getCaptionId() const {
            return _captionId;
        }
    };

private:
	static const float SOUND_MIN_VALUE;
	static const float SOUND_MAX_VALUE;
	static const float MUSIC_MIN_VALUE;
	static const float MUSIC_MAX_VALUE;
	static const float SENSITIVITY_MIN_VALUE;
	static const float SENSITIVITY_MAX_VALUE;
	static const float PIXELS_PER_MILLIMETER_MIN_VALUE;
	static const float PIXELS_PER_MILLIMETER_MAX_VALUE;
    static const char* RENDER_DISTANCE_NAMES[];
    static const char* DIFFICULTY_NAMES[];
    static const char* GUI_SCALE[];
	static const int DIFFICULY_LEVELS[];
public:
	static bool debugGl;

	float music;
    float sound;
    //note: sensitivity is transformed in Options::update
    float sensitivity;
    bool invertYMouse;
    int viewDistance;
    bool bobView;
    bool anaglyph3d;
    bool limitFramerate;
    bool fancyGraphics;
    bool ambientOcclusion;
	bool useMouseForDigging;
	bool isLeftHanded;
	bool destroyVibration;
    //std::string skin;

    KeyMapping keyUp;
    KeyMapping keyLeft;
    KeyMapping keyDown;
    KeyMapping keyRight;
    KeyMapping keyJump;
    KeyMapping keyBuild;
    KeyMapping keyDrop;
    KeyMapping keyChat;
    KeyMapping keyFog;
    KeyMapping keySneak;
	KeyMapping keyCraft;
	KeyMapping keyDestroy;
	KeyMapping keyUse;

	KeyMapping keyMenuNext;
	KeyMapping keyMenuPrevious;
	KeyMapping keyMenuOk;
	KeyMapping keyMenuCancel;

    KeyMapping* keyMappings[16];

	/*protected*/ Minecraft* minecraft;
    ///*private*/ File optionsFile;

    int difficulty;
    bool hideGui;
    bool thirdPersonView;
    bool renderDebug;

    bool isFlying;
    bool smoothCamera;
    bool fixedCamera;
    float flySpeed;
    float cameraSpeed;
    int guiScale;
	std::string username;

	bool serverVisible;
	bool isJoyTouchArea;
	bool useTouchScreen;
	float pixelsPerMillimeter;
    Options(Minecraft* minecraft, const std::string& workingDirectory)
	:	minecraft(minecraft)
	{
        //optionsFile = /*new*/ File(workingDirectory, "options.txt");
        initDefaultValues();

		load();
    }

	Options()
	:	minecraft(NULL)
	{
		
	}

	void initDefaultValues();

    std::string getKeyDescription(int i) {
        //Language language = Language.getInstance();
        //return language.getElement(keyMappings[i].name);
		return "Options::getKeyDescription not implemented";
    }

    std::string getKeyMessage(int i) {
        //return Keyboard.getKeyName(keyMappings[i].key);
		return "Options::getKeyMessage not implemented";
    }

    void setKey(int i, int key) {
        keyMappings[i]->key = key;
        save();
    }

    void set(const Option* item, float value) {
        if (item == &Option::MUSIC) {
            music = value;
            //minecraft.soundEngine.updateOptions();
        } else if (item == &Option::SOUND) {
            sound = value;
            //minecraft.soundEngine.updateOptions();
        } else if (item == &Option::SENSITIVITY) {
            sensitivity = value;
		} else if (item == &Option::PIXELS_PER_MILLIMETER) {
			 pixelsPerMillimeter = value;
		}
		notifyOptionUpdate(item, value);
    }
	void set(const Option* item, int value) {
		if(item == &Option::DIFFICULTY) {
			difficulty = value;
		}
		notifyOptionUpdate(item, value);
	}

    void toggle(const Option* option, int dir) {
        if (option == &Option::INVERT_MOUSE)	invertYMouse = !invertYMouse;
        if (option == &Option::RENDER_DISTANCE) viewDistance = (viewDistance + dir) & 3;
        if (option == &Option::GUI_SCALE)		guiScale = (guiScale + dir) & 3;
        if (option == &Option::VIEW_BOBBING)	bobView = !bobView;
		if (option == &Option::THIRD_PERSON)	thirdPersonView = !thirdPersonView;
		if (option == &Option::HIDE_GUI)		hideGui = !hideGui;
		if (option == &Option::SERVER_VISIBLE)		serverVisible = !serverVisible;
		if (option == &Option::LEFT_HANDED) isLeftHanded = !isLeftHanded;
		if (option == &Option::USE_TOUCHSCREEN) useTouchScreen = !useTouchScreen;
		if (option == &Option::USE_TOUCH_JOYPAD) isJoyTouchArea = !isJoyTouchArea;
		if (option == &Option::DESTROY_VIBRATION) destroyVibration = !destroyVibration;
		if (option == &Option::ANAGLYPH) {
            anaglyph3d = !anaglyph3d;
            //minecraft->textures.reloadAll();
        }
        if (option == &Option::LIMIT_FRAMERATE) limitFramerate = !limitFramerate;
        if (option == &Option::DIFFICULTY) difficulty = (difficulty + dir) & 3;
        if (option == &Option::GRAPHICS) {
            fancyGraphics = !fancyGraphics;
            //minecraft->levelRenderer.allChanged();
        }
        if (option == &Option::AMBIENT_OCCLUSION) {
            ambientOcclusion = !ambientOcclusion;
            //minecraft->levelRenderer.allChanged();
        }
		notifyOptionUpdate(option, getBooleanValue(option));
        save();
    }

	int getIntValue(const Option* item) {
		if(item == &Option::DIFFICULTY) return difficulty;
		return 0;
	}

    float getProgressValue(const Option* item) {
        if (item == &Option::MUSIC) return music;
        if (item == &Option::SOUND) return sound;
        if (item == &Option::SENSITIVITY) return sensitivity;
		if (item == &Option::PIXELS_PER_MILLIMETER) return pixelsPerMillimeter;
        return 0;
    }

    bool getBooleanValue(const Option* item) {
        if (item == &Option::INVERT_MOUSE)
            return invertYMouse;
        if (item == &Option::VIEW_BOBBING)
            return bobView;
        if (item == &Option::ANAGLYPH)
            return anaglyph3d;
        if (item == &Option::LIMIT_FRAMERATE)
            return limitFramerate;
        if (item == &Option::AMBIENT_OCCLUSION)
            return ambientOcclusion;
        if (item == &Option::THIRD_PERSON)
            return thirdPersonView;
        if (item == &Option::HIDE_GUI)
            return hideGui;
		if (item == &Option::THIRD_PERSON)
			return thirdPersonView;
		if (item == &Option::SERVER_VISIBLE)
			return serverVisible;
		if (item == &Option::LEFT_HANDED)
			return isLeftHanded;
		if (item == &Option::USE_TOUCHSCREEN)
			return useTouchScreen;
		if (item == &Option::USE_TOUCH_JOYPAD)
			return isJoyTouchArea;
		if (item == &Option::DESTROY_VIBRATION)
			return destroyVibration;
		return false;
	}

	float getProgrssMin(const Option* item) {
		if (item == &Option::MUSIC) return MUSIC_MIN_VALUE;
		if (item == &Option::SOUND) return SOUND_MIN_VALUE;
		if (item == &Option::SENSITIVITY) return SENSITIVITY_MIN_VALUE;
		if (item == &Option::PIXELS_PER_MILLIMETER) return PIXELS_PER_MILLIMETER_MIN_VALUE;
		return 0;
	}

	float getProgrssMax(const Option* item) {
		if (item == &Option::MUSIC) return MUSIC_MAX_VALUE;
		if (item == &Option::SOUND) return SOUND_MAX_VALUE;
		if (item == &Option::SENSITIVITY) return SENSITIVITY_MAX_VALUE;
		if (item == &Option::PIXELS_PER_MILLIMETER) return PIXELS_PER_MILLIMETER_MAX_VALUE;
		return 1.0f;
	} 

	std::string getMessage(const Option* item);

	void update();
    void load();
    void save();
	void addOptionToSaveOutput(StringVector& stringVector, std::string name, bool boolValue);
	void addOptionToSaveOutput(StringVector& stringVector, std::string name, float floatValue);
	void addOptionToSaveOutput(StringVector& stringVector, std::string name, int intValue);
	void notifyOptionUpdate(const Option* option, bool value);
	void notifyOptionUpdate(const Option* option, float value);
	void notifyOptionUpdate(const Option* option, int value);
private:
    static bool readFloat(const std::string& string, float& value);
    static bool readInt(const std::string& string, int& value);
	static bool readBool(const std::string& string, bool& value);

private:
	OptionsFile optionsFile;
	
};

#endif /*NET_MINECRAFT_CLIENT__Options_H__*/
