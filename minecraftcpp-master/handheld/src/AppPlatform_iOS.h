#ifndef APPPLATFORM_IOS_H__
#define APPPLATFORM_IOS_H__

#include "AppPlatform.h"
#include "client/renderer/gles.h"
#include "platform/log.h"
#include <cmath>
#include <fstream>
#include <sstream>

@class minecraftpeViewController;

class AppPlatform_iOS: public AppPlatform
{
	typedef AppPlatform super;
public:
    AppPlatform_iOS(minecraftpeViewController* vc) {
        _viewController = vc;
        srand(time(0));

        LOGI("ViewController in AppPlatform: %p\n", _viewController);
    }

    void setBasePath(const std::string& bp) { _basePath = bp; }
    
    void saveScreenshot(const std::string& filename, int glWidth, int glHeight) {
        //@todo
    }

    __inline unsigned int rgbToBgr(unsigned int p) {
        return (p & 0xff00ff00) | ((p >> 16) & 0xff) | ((p << 16) & 0xff0000);
    }

	virtual void showDialog(int dialogId);
	virtual int getUserInputStatus();
	virtual StringVector getUserInput();
    
    TextureData loadTexture(const std::string& filename_, bool textureFolder);

	virtual BinaryBlob readAssetFile(const std::string& filename);
    
    std::string getDateString(int s);

	virtual int checkLicense() {
        return 0;
		static int _z = 20;
		_z--;
		if (_z < 0) return 0;
		//if (_z < 0) return 107;
		return -2;
	}
    
    virtual void buyGame();

	virtual int getScreenWidth();
	virtual int getScreenHeight();
    virtual float getPixelsPerMillimeter();
    
	virtual bool isTouchscreen();
    virtual void vibrate(int milliSeconds);
    
	virtual bool isNetworkEnabled(bool onlyWifiAllowed);
    
	virtual StringVector getOptionStrings();

    virtual bool isPowerVR() { return false; }
    virtual bool isSuperFast();
    virtual void showKeyboard();
    virtual void hideKeyboard();
	virtual void isPowerVR();
private:
    
    std::string _basePath;
    minecraftpeViewController* _viewController;
};

#endif /*APPPLATFORM_IOS_H__*/
