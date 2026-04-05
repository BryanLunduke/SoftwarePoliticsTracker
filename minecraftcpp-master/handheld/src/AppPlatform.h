#ifndef APPPLATFORM_H__
#define APPPLATFORM_H__

#include <vector>
#include <string>
#include <cstring>
#include "client/renderer/TextureData.h"

typedef std::vector<std::string> StringVector;

/*
typedef struct UserInput
{
    static const int STATUS_INVALID = -1;
    static const int STATUS_NOTINITED = -2;
    static const int STATUS_OK = 1;
    static const int STATUS_CANCEL = 0;

    UserInput(int id)
    :   _id(id),
        status(STATUS_NOTINITED)
    {}
    UserInput(int id, int status)
    :   _id(id),
        status(status)
    {}
    int getId() { return _id; }

    int status;
private:
    int _id;
} UserInput;


class UserInputStatus {
	int _status;
public:
	UserInputStatus(int status)
	:	_status(status)
	{}
	bool isAnswered() { return _status >= 0; }
	bool isOk() { return _status == UserInput::STATUS_OK; }
	bool isCancel() { return _status == UserInput::STATUS_CANCEL; }
};
*/

class BinaryBlob {
public:
	BinaryBlob()
	:	data(NULL),
		size(-1) {}

	BinaryBlob(unsigned char* data, unsigned int size)
	:	data(data),
		size(size) {}

	unsigned char* data;
	int size;
};

class PlatformStringVars {
public:
	static const int DEVICE_BUILD_MODEL = 0;
};

class AppPlatform
{
public:
	AppPlatform() : keyboardVisible(false) {}
    virtual ~AppPlatform() {}

	virtual void saveScreenshot(const std::string& filename, int glWidth, int glHeight) {}
	virtual TextureData loadTexture(const std::string& filename_, bool textureFolder) { return TextureData(); }

    virtual void playSound(const std::string& fn, float volume, float pitch) {}

	virtual void showDialog(int dialogId) {}
    virtual void createUserInput() {}
	
	bool is_big_endian(void)  {
		union {
			unsigned int i;
			char c[4];
		} bint = {0x01020304};
		return bint.c[0] == 1;
	} 

	void createUserInput(int dialogId)
	{
		showDialog(dialogId);
		createUserInput();
	}
	virtual int getUserInputStatus() { return 0; }
	virtual StringVector getUserInput() { return StringVector(); }

	virtual std::string getDateString(int s) { return ""; }
	//virtual void createUserInputScreen(const char* types) {}

    virtual int checkLicense() { return 0; }
	virtual bool hasBuyButtonWhenInvalidLicense() { return false; }

	virtual void uploadPlatformDependentData(int id, void* data) {}
	virtual BinaryBlob readAssetFile(const std::string& filename) { return BinaryBlob(); }
	virtual void _tick() {}

	virtual int getScreenWidth() { return 854; }
	virtual int getScreenHeight() { return 480; }
    virtual float getPixelsPerMillimeter() { return 10; }

	virtual bool isNetworkEnabled(bool onlyWifiAllowed) { return true; }

	virtual bool isPowerVR() {
		return false;
	}
	virtual int getKeyFromKeyCode(int keyCode, int metaState, int deviceId) {return 0;}
#ifdef __APPLE__
    virtual bool isSuperFast() = 0;
#endif

	virtual void buyGame() {}

	virtual void finish() {}
	
	virtual bool supportsTouchscreen() { return true; }
	
	virtual void vibrate(int milliSeconds) {}

	virtual std::string getPlatformStringVar(int stringId) {
		return "<getPlatformStringVar NotImplemented>";
	}

	virtual void showKeyboard() {
		keyboardVisible = true;
	}
	virtual void hideKeyboard() {
		keyboardVisible = false;
	}
	virtual bool isKeyboardVisible() {return keyboardVisible;}
protected:
	bool keyboardVisible;
};

#endif /*APPPLATFORM_H__*/
