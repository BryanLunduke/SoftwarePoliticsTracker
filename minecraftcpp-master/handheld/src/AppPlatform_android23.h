#include "AppPlatform_android.h"
#include <android/asset_manager.h>
#include <android/native_activity.h>

class AppPlatform_android23 : public AppPlatform_android
{
	typedef AppPlatform_android super;
public:
	AppPlatform_android23()
	:	_assetManager(NULL)
	{
	}

	// If we're using Android 2.3+, try reading assets from NDK at first.
	// If that doesn't work, read through java/JNI as usual.
	BinaryBlob readAssetFile(const std::string& filename) {
		if (!_isInited)
			return BinaryBlob();

		if (_assetManager != NULL) {
			AAsset* asset = AAssetManager_open(_assetManager, filename.c_str(), AASSET_MODE_BUFFER);
			if (asset != NULL) {
				const int   len = AAsset_getLength(asset);
				const void* buf = len > 0? AAsset_getBuffer(asset) : NULL;

				BinaryBlob blob;
				if (buf != NULL) {
					blob = BinaryBlob(new unsigned char[len], len);
					memcpy(blob.data, buf, len);
				}
				AAsset_close(asset);
				if (blob.data)
					return blob;
			}
		}
		return super::readAssetFile(filename);
	}


	// Another init method... added to read data from the activity, and setup constants
	// @note: This is called after instance is set from the outside, BUT this
	//        will be rewritten later on anyway
	void initWithActivity(struct ANativeActivity* activity) {
		_assetManager = activity->assetManager;
	}

private:
	AAssetManager* _assetManager;
};
