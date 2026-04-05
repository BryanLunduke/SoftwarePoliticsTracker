#import <UIKit/UIKit.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

@interface PVRTexture : NSObject
{
	NSMutableArray *_imageData;
	
	GLuint _name;
	uint32_t _width, _height;
	GLenum _internalFormat;
	BOOL _hasAlpha;
}

- (id)initWithContentsOfFile:(NSString *)path;
- (id)initWithContentsOfURL:(NSURL *)url;
+ (id)pvrTextureWithContentsOfFile:(NSString *)path;
+ (id)pvrTextureWithContentsOfURL:(NSURL *)url;

@property (readonly) GLuint name;
@property (readonly) uint32_t width;
@property (readonly) uint32_t height;
@property (readonly) GLenum internalFormat;
@property (readonly) BOOL hasAlpha;

@end
