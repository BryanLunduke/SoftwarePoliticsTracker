#ifndef NET_MINECRAFT_CLIENT_RENDERER__TextureData_H__
#define NET_MINECRAFT_CLIENT_RENDERER__TextureData_H__

enum TextureFormat
{
    TEXF_UNCOMPRESSED_8888,
    TEXF_UNCOMPRESSED_565,
    TEXF_UNCOMPRESSED_5551,
    TEXF_UNCOMPRESSED_4444,
    TEXF_COMPRESSED_PVRTC_565,
    TEXF_COMPRESSED_PVRTC_5551,
    TEXF_COMPRESSED_PVRTC_4444
};

typedef struct TextureData {
	TextureData()
	:	w(0),
		h(0),
		data(NULL),
        numBytes(0),
		transparent(true),
		memoryHandledExternally(false),
        format(TEXF_UNCOMPRESSED_8888),
        identifier(-1)
	{}

	int w, h;
	unsigned char* data;
    int numBytes;
	bool transparent;
	bool memoryHandledExternally;

    TextureFormat format;
    int identifier; 
} TextureData;

#endif /*NET_MINECRAFT_CLIENT_RENDERER__Textures_H__*/
