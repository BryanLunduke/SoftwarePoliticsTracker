#include "AppPlatform_iOS.h"
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>

#import "../project/iosproj/minecraftpe/minecraftpeViewController.h"
#import "../project/iosproj/minecraftpe/dialogs/BaseDialogController.h"
#import "../project/iosproj/minecraftpe/PVRTexture.h"
#import "client/gui/screens/DialogDefinitions.h"

#import "terrain_565.h"

typedef unsigned int PVRTuint32;

struct PVR_Texture_Header
{
	PVRTuint32 dwHeaderSize;			/*!< size of the structure */
	PVRTuint32 dwHeight;				/*!< height of surface to be created */
	PVRTuint32 dwWidth;				/*!< width of input surface */
	PVRTuint32 dwMipMapCount;			/*!< number of mip-map levels requested */
	PVRTuint32 dwpfFlags;				/*!< pixel format flags */
	PVRTuint32 dwTextureDataSize;		/*!< Total size in bytes */
	PVRTuint32 dwBitCount;			/*!< number of bits per pixel  */
	PVRTuint32 dwRBitMask;			/*!< mask for red bit */
	PVRTuint32 dwGBitMask;			/*!< mask for green bits */
	PVRTuint32 dwBBitMask;			/*!< mask for blue bits */
	PVRTuint32 dwAlphaBitMask;		/*!< mask for alpha channel */
	PVRTuint32 dwPVR;					/*!< magic number identifying pvr file */
	PVRTuint32 dwNumSurfs;			/*!< the number of surfaces present in the pvr */
} ;

void AppPlatform_iOS::showDialog(int dialogId) {
    if (dialogId == DialogDefinitions::DIALOG_CREATE_NEW_WORLD) {
        [_viewController showDialog_CreateWorld];
    }
    if (dialogId == DialogDefinitions::DIALOG_MAINMENU_OPTIONS) {
        [_viewController showDialog_MainMenuOptions];
    }
    if (dialogId == DialogDefinitions::DIALOG_RENAME_MP_WORLD) {
        [_viewController showDialog_RenameMPWorld];
    }
    if (dialogId == DialogDefinitions::DIALOG_DEMO_FEATURE_DISABLED) {
        UIAlertView *a = [[UIAlertView alloc]
            initWithTitle:@"" 
            message:@"Feature not enabled for this demo" 
            delegate:nil 
            cancelButtonTitle:@"OK"
            otherButtonTitles:nil];
        [a show];
        [a release];
    }
}

TextureData AppPlatform_iOS::loadTexture(const std::string& filename_, bool textureFolder)
{
    TextureData out;
    out.memoryHandledExternally = false;

    std::string filename = filename_;
    size_t dotp = filename.rfind(".");
    size_t slashp = filename.rfind("/");
    if (dotp != std::string::npos || slashp != std::string::npos) {
        if (slashp == std::string::npos) slashp = -1;
        filename = filename.substr(slashp+1, dotp-(slashp+1));
    }

//    if (filename == "terrain" || filename_[dotp+2] == 'v') { // @fix
//        //NSString *path = [[NSBundle mainBundle] pathForResource:[[NSString alloc] initWithUTF8String:filename.c_str()] ofType:@"pvr4"];
//        
//        //FILE* fp = fopen([path UTF8String], "rb");
//        int fp = 1;
//        if (fp) {
//            PVR_Texture_Header header;
//            header = *((PVR_Texture_Header*)terrain_565);
//            //fread(&header, 1, sizeof(PVR_Texture_Header), fp);
//            int numBytes = header.dwTextureDataSize;
//            //out.data = new unsigned char[numBytes];
//            out.data = (unsigned char*)&terrain_565[header.dwHeaderSize];
//            out.memoryHandledExternally = true;
//            out.numBytes = numBytes;
//            out.transparent = (header.dwAlphaBitMask != 0);
//            //fread(out.data, 1, numBytes, fp);
//            out.w = header.dwWidth;
//            out.h = header.dwHeight;
//            LOGI("Size of file: %d (%d, %d) - %x,%x,%x,%x\n", out.numBytes, out.w, out.h,
//                 header.dwRBitMask,
//                 header.dwGBitMask,
//                 header.dwBBitMask,
//                 header.dwAlphaBitMask);
//            out.format = TEXF_UNCOMPRESSED_565;// TEXF_COMPRESSED_PVRTC_5551;
//            //fclose(fp);
//        }
//        
////        PVRTexture* tex = [PVRTexture pvrTextureWithContentsOfFile:path];
////        //NSLog(@"path: %@, tex: %p, name: %d\n", path, tex, [tex name]);
////        out.identifier = [tex name];
////        out.w = [tex width];
////        out.h = [tex height];
////        GLuint texId;
////        //PVRTTextureLoadFromPVR([path UTF8String], &texId);
////        out.identifier = texId;
//        return out;
//    }
    
    NSString *p = [[NSString alloc] initWithUTF8String:filename.c_str()];
    NSString *path =  [[NSBundle mainBundle] pathForResource:p ofType:@"png"];
    [p release];
    NSData *texData = [[NSData alloc] initWithContentsOfFile:path];
    UIImage *image = [[UIImage alloc] initWithData:texData];

    if (image != nil) {
        // Get Image size
        out.w = CGImageGetWidth(image.CGImage);
        out.h = CGImageGetHeight(image.CGImage);
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        // Allocate memory for image
        out.data = new unsigned char[4 * out.w * out.h];
        CGContextRef imgcontext = CGBitmapContextCreate( out.data, out.w, out.h, 8, 4 * out.w, colorSpace, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big );
        CGColorSpaceRelease( colorSpace );
        CGContextClearRect( imgcontext, CGRectMake( 0, 0, out.w, out.h ) );
        CGContextTranslateCTM( imgcontext, 0, 0);//height - height );
        CGContextDrawImage( imgcontext, CGRectMake( 0, 0, out.w, out.h ), image.CGImage );
        CGContextRelease(imgcontext);
    } else {
        LOGI("Couldn't find file: %s\n", filename.c_str());

        if ("this is idiotic but temporary") {
            out.w = 16;
            out.h = 16;
            bool isTerrain = (filename.find("terrain") != std::string::npos);
            int numPixels = out.w * out.h;
            out.data = new unsigned char[4 * numPixels];
            if (isTerrain) {
                for (int i = 0; i < numPixels; ++i) {
                    unsigned int color = 0xff000000 | ((rand() & 0xff) << 16) | (rand() & 0xffff);
                    *((int*)(&out.data[4*i])) = color;
                }
            } else {
                unsigned int color = 0xff000000 | ((rand() & 0xff) << 16) | (rand() & 0xffff);
                for (int i = 0; i < numPixels; ++i) {
                    *((int*)(&out.data[4*i])) = color;
                }
            }
        }
    }
    [image release];
    [texData release];
    
    return out;
}

BinaryBlob AppPlatform_iOS::readAssetFile(const std::string& filename_) {
    std::string filename = filename_;
    size_t dotp = filename.rfind(".");
    size_t slashp = filename.rfind("/");
    std::string ext;
    if (dotp != std::string::npos || slashp != std::string::npos) {
        // Get file extension
        if (dotp != std::string::npos) {
            ext = filename.substr(dotp+1);
        }
        if (slashp == std::string::npos) slashp = -1;
        filename = filename.substr(slashp+1, dotp-(slashp+1));
    }
    NSString *rext = [NSString stringWithUTF8String:ext.c_str()];
    NSString *p = [[NSString alloc] initWithUTF8String:filename.c_str()];
    NSString *path =  [[NSBundle mainBundle] pathForResource:p ofType:rext];
    [p release];
    NSData *data = [NSData dataWithContentsOfFile:path];
    if (!data)
        return BinaryBlob();

    unsigned int numBytes = [data length];
    unsigned char* bytes = new unsigned char[numBytes];
    memcpy(bytes, [data bytes], numBytes);
    return BinaryBlob(bytes, numBytes);
}

void AppPlatform_iOS::buyGame() {
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"http://phobos.apple.com/WebObjects/MZStore.woa/wa/viewSoftware?id=479516143&mt=8"]];
}

std::string AppPlatform_iOS::getDateString(int s) {

    NSDate* date = [NSDate dateWithTimeIntervalSince1970:s];
    
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    //dateFormatter.dateFormat = @"yyyy-MM-dd HH:mm";

    [dateFormatter setDateStyle:NSDateFormatterMediumStyle];
    [dateFormatter setTimeStyle:NSDateFormatterShortStyle];
    
    //NSTimeZone *gmt = [NSTimeZone timeZoneWithAbbreviation:@"GMT"];
    //[dateFormatter setTimeZone:gmt];
    NSString *timeStamp = [dateFormatter stringFromDate:date];
    [dateFormatter release];

    return std::string( [timeStamp UTF8String] );
}

int AppPlatform_iOS::getScreenWidth()  { return 480; }
int AppPlatform_iOS::getScreenHeight() { return 320; }

float AppPlatform_iOS::getPixelsPerMillimeter() {
    // @note: @retina has a much higher density, however,
    //        we use 480x320 for the OpenGL context size
    BOOL isIpad = (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);
    if (isIpad) {
        return 5.1968503937007879f * _viewController->viewScale;
    } else {
        return 6.4173228346456694f * _viewController->viewScale;
    }
}

bool AppPlatform_iOS::isTouchscreen()  { return true; }

void AppPlatform_iOS::vibrate(int ms) {
    // Note: In iOS 4, there's no way to set length of the vibration, so it's useless
    //AudioServicesPlaySystemSound (kSystemSoundID_Vibrate);
}

int AppPlatform_iOS::getUserInputStatus() {
    return [_viewController getUserInputStatus];
}

StringVector AppPlatform_iOS::getUserInput() {
    return [_viewController getUserInput];
}

StringVector AppPlatform_iOS::getOptionStrings() {
    //@options
    StringVector options;
    NSDictionary* d = [[NSUserDefaults standardUserDefaults] dictionaryRepresentation];
    
    for( NSString *key in d )
    {
        if ([key hasPrefix:@"mp_"]
         || [key hasPrefix:@"gfx_"]
         || [key hasPrefix:@"ctrl_"]
         || [key hasPrefix:@"feedback_"]
         || [key hasPrefix:@"game_"] ) {
            id value = [d objectForKey: key];
            options.push_back([key UTF8String]);
            options.push_back([[value description] UTF8String]);
            //LOGI("Added strings: %s\n", options[options.size()-1].c_str());
        }
    }
    return options;
}

bool AppPlatform_iOS::isSuperFast() {
    const char* s = (const char*)glGetString(GL_RENDERER);
    if (!s) return false;
    
    return (strstr(s, "SGX") != NULL) && (strstr(s, "543") != NULL);
}

bool AppPlatform_iOS::isNetworkEnabled(bool onlyWifiAllowed) {
    return true;
    /*
    Reachability *reachability = [Reachability reachabilityForInternetConnection];
    [reachability startNotifier];
    
    NetworkStatus status = [reachability currentReachabilityStatus];
    bool success = (status == ReachableViaWiFiNetwork);
    if (!onlyWifiAllowed && !success)
        success = (status == ReachableViaWWAN);

    [reachability stopNotifier];
    return success;
    */
}

void AppPlatform_iOS::showKeyboard() {
    [_viewController showKeyboard];
	super::showKeyboard();
}

void AppPlatform_iOS::hideKeyboard() {
    [_viewController hideKeyboard];
	super::hideKeyboard();
}
void AppPlatform_iOS::isPowerVR() {
	const char* s = (const char*)glGetString(GL_RENDERER);
	if (!s) return false;
	return strstr(s, "SGX") != NULL;
}