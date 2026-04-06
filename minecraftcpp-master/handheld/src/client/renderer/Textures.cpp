#include "Textures.h"

#include "TextureData.h"
#include "ptexture/DynamicTexture.h"
#include "../Options.h"
#include "../../platform/time.h"
#include "../../AppPlatform.h"

/*static*/ int  Textures::textureChanges = 0;
/*static*/ bool Textures::MIPMAP = false;

Textures::Textures( Options* options_, AppPlatform* platform_ )
:	clamp(false),
	blur(false),
	options(options_),
	platform(platform_),
	lastBoundTexture(Textures::InvalidId)
{
}

Textures::~Textures()
{
	clear();

	for (unsigned int i = 0; i < dynamicTextures.size(); ++i)
		delete dynamicTextures[i];
}

void Textures::clear()
{
	for (TextureMap::iterator it = idMap.begin(); it != idMap.end(); ++it) {
		if (it->second != Textures::InvalidId)
			glDeleteTextures(1, &it->second);
	}
	for (TextureImageMap::iterator it = loadedImages.begin(); it != loadedImages.end(); ++it) {
		if (!(it->second).memoryHandledExternally)
			delete[] (it->second).data;
	}
	idMap.clear();
	loadedImages.clear();

	lastBoundTexture = Textures::InvalidId;
}

TextureId Textures::loadAndBindTexture( const std::string& resourceName )
{
	//static Stopwatch t;

	//t.start();
	TextureId id = loadTexture(resourceName);
	//t.stop();
	if (id != Textures::InvalidId)
		bind(id);

	//t.printEvery(1000);

	return id;
}

TextureId Textures::loadTexture( const std::string& resourceName, bool inTextureFolder /* = true */ )
{
	TextureMap::iterator it = idMap.find(resourceName);
	if (it != idMap.end())
		return it->second;

	TextureData texdata = platform->loadTexture(resourceName, inTextureFolder);
	if (texdata.data)
		return assignTexture(resourceName, texdata);
    else if (texdata.identifier != InvalidId) {
        //LOGI("Adding id: %d for %s\n", texdata.identifier, resourceName.c_str());
		idMap.insert(std::make_pair(resourceName, texdata.identifier));
    }
	else {
		idMap.insert(std::make_pair(resourceName, Textures::InvalidId));
		//loadedImages.insert(std::make_pair(InvalidId, texdata));
	}
	return Textures::InvalidId;
}

TextureId Textures::assignTexture( const std::string& resourceName, const TextureData& img )
{
	TextureId id;
	glGenTextures(1, &id);

	bind(id);

	if (MIPMAP) {
		glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	} else {
		glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	if (blur) {
		glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	if (clamp) {
		glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else {
		glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

    switch (img.format)
    {
        case TEXF_COMPRESSED_PVRTC_4444:
        case TEXF_COMPRESSED_PVRTC_565:
        case TEXF_COMPRESSED_PVRTC_5551:
        {
#if defined(__APPLE__)
            int fmt = img.transparent? GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG : GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, fmt, img.w, img.h, 0, img.numBytes, img.data);
#endif
            break;
        }

        default:
            const GLint mode = img.transparent? GL_RGBA : GL_RGB;

            if (img.format == TEXF_UNCOMPRESSED_565) {
                glTexImage2D2(GL_TEXTURE_2D, 0, mode, img.w, img.h, 0, mode, GL_UNSIGNED_SHORT_5_6_5, img.data);
            }
            else if (img.format == TEXF_UNCOMPRESSED_4444) {
                glTexImage2D2(GL_TEXTURE_2D, 0, mode, img.w, img.h, 0, mode, GL_UNSIGNED_SHORT_4_4_4_4, img.data);
            }
            else if (img.format == TEXF_UNCOMPRESSED_5551) {
                glTexImage2D2(GL_TEXTURE_2D, 0, mode, img.w, img.h, 0, mode, GL_UNSIGNED_SHORT_5_5_5_1, img.data);
            }
            else {
                glTexImage2D2(GL_TEXTURE_2D, 0, mode, img.w, img.h, 0, mode, GL_UNSIGNED_BYTE, img.data);
            }
            break;
    }

    //LOGI("Adding id: %d to map\n", id);
	idMap.insert(std::make_pair(resourceName, id));
	loadedImages.insert(std::make_pair(id, img));

	return id;
}

const TextureData* Textures::getTemporaryTextureData( TextureId id )
{
	TextureImageMap::iterator it = loadedImages.find(id);
	if (it == loadedImages.end())
		return NULL;

	return &it->second;
}

void Textures::tick(bool uploadToGraphicsCard)
{
	for (unsigned int i = 0; i < dynamicTextures.size(); ++i ) {
		DynamicTexture* tex = dynamicTextures[i];
		tex->tick();

        if (uploadToGraphicsCard) {
            tex->bindTexture(this);
		    for (int xx = 0; xx < tex->replicate; xx++)
		    for (int yy = 0; yy < tex->replicate; yy++) {
			    glTexSubImage2D2(GL_TEXTURE_2D, 0, tex->tex % 16 * 16 + xx * 16,
				    tex->tex / 16 * 16 + yy * 16, 16, 16,
				    GL_RGBA, GL_UNSIGNED_BYTE, tex->pixels);
		    }
        }
	}
}

void Textures::addDynamicTexture( DynamicTexture* dynamicTexture )
{
	dynamicTextures.push_back(dynamicTexture);
	dynamicTexture->tick();
}

void Textures::reloadAll()
{
	//TexturePack skin = skins.selected;

	//for (int id : loadedImages.keySet()) {
	//    BufferedImage image = loadedImages.get(id);
	//    loadTexture(image, id);
	//}

	////for (HttpTexture httpTexture : httpTextures.values()) {
	////    httpTexture.isLoaded = false;
	////}

	//for (std::string name : idMap.keySet()) {
	//    try {
	//        BufferedImage image;
	//        if (name.startsWith("##")) {
	//            image = makeStrip(readImage(skin.getResource(name.substring(2))));
	//        } else if (name.startsWith("%clamp%")) {
	//            clamp = true;
	//            image = readImage(skin.getResource(name.substring(7)));
	//        } else if (name.startsWith("%blur%")) {
	//            blur = true;
	//            image = readImage(skin.getResource(name.substring(6)));
	//        } else {
	//            image = readImage(skin.getResource(name));
	//        }
	//        int id = idMap.get(name);
	//        loadTexture(image, id);
	//        blur = false;
	//        clamp = false;
	//    } catch (IOException e) {
	//        e.printStackTrace();
	//    }
	//}
}

int Textures::smoothBlend( int c0, int c1 )
{
	int a0 = (int) (((c0 & 0xff000000) >> 24)) & 0xff;
	int a1 = (int) (((c1 & 0xff000000) >> 24)) & 0xff;
	return ((a0 + a1) >> 1 << 24) + (((c0 & 0x00fefefe) + (c1 & 0x00fefefe)) >> 1);
}

int Textures::crispBlend( int c0, int c1 )
{
	int a0 = (int) (((c0 & 0xff000000) >> 24)) & 0xff;
	int a1 = (int) (((c1 & 0xff000000) >> 24)) & 0xff;

	int a = 255;
	if (a0 + a1 == 0) {
		a0 = 1;
		a1 = 1;
		a = 0;
	}

	int r0 = ((c0 >> 16) & 0xff) * a0;
	int g0 = ((c0 >> 8) & 0xff) * a0;
	int b0 = ((c0) & 0xff) * a0;

	int r1 = ((c1 >> 16) & 0xff) * a1;
	int g1 = ((c1 >> 8) & 0xff) * a1;
	int b1 = ((c1) & 0xff) * a1;

	int r = (r0 + r1) / (a0 + a1);
	int g = (g0 + g1) / (a0 + a1);
	int b = (b0 + b1) / (a0 + a1);

	return (a << 24) | (r << 16) | (g << 8) | b;
}

///*public*/ int loadHttpTexture(std::string url, std::string backup) {
//    HttpTexture texture = httpTextures.get(url);
//    if (texture != NULL) {
//        if (texture.loadedImage != NULL && !texture.isLoaded) {
//            if (texture.id < 0) {
//                texture.id = getTexture(texture.loadedImage);
//            } else {
//                loadTexture(texture.loadedImage, texture.id);
//            }
//            texture.isLoaded = true;
//        }
//    }
//    if (texture == NULL || texture.id < 0) {
//        if (backup == NULL) return -1;
//        return loadTexture(backup);
//    }
//    return texture.id;
//}

//HttpTexture addHttpTexture(std::string url, HttpTextureProcessor processor) {
//    HttpTexture texture = httpTextures.get(url);
//    if (texture == NULL) {
//        httpTextures.put(url, /*new*/ HttpTexture(url, processor));
//    } else {
//        texture.count++;
//    }
//    return texture;
//}

//void removeHttpTexture(std::string url) {
//    HttpTexture texture = httpTextures.get(url);
//    if (texture != NULL) {
//        texture.count--;
//        if (texture.count == 0) {
//            if (texture.id >= 0) releaseTexture(texture.id);
//            httpTextures.remove(url);
//        }
//    }
//}

//void tick() {
//	for (int i = 0; i < dynamicTextures.size(); i++) {
//		DynamicTexture dynamicTexture = dynamicTextures.get(i);
//		dynamicTexture.anaglyph3d = options.anaglyph3d;
//		dynamicTexture.tick();
//
//		pixels.clear();
//		pixels.put(dynamicTexture.pixels);
//		pixels.position(0).limit(dynamicTexture.pixels.length);
//
//		dynamicTexture.bindTexture(this);
//
//		for (int xx = 0; xx < dynamicTexture.replicate; xx++)
//			for (int yy = 0; yy < dynamicTexture.replicate; yy++) {
//
//				glTexSubImage2D2(GL_TEXTURE_2D, 0, dynamicTexture.tex % 16 * 16 + xx * 16, dynamicTexture.tex / 16 * 16 + yy * 16, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
//				if (MIPMAP) {
//					for (int level = 1; level <= 4; level++) {
//						int os = 16 >> (level - 1);
//						int s = 16 >> level;
//
//						for (int x = 0; x < s; x++)
//							for (int y = 0; y < s; y++) {
//								int c0 = pixels.getInt(((x * 2 + 0) + (y * 2 + 0) * os) * 4);
//								int c1 = pixels.getInt(((x * 2 + 1) + (y * 2 + 0) * os) * 4);
//								int c2 = pixels.getInt(((x * 2 + 1) + (y * 2 + 1) * os) * 4);
//								int c3 = pixels.getInt(((x * 2 + 0) + (y * 2 + 1) * os) * 4);
//								int col = smoothBlend(smoothBlend(c0, c1), smoothBlend(c2, c3));
//								pixels.putInt((x + y * s) * 4, col);
//							}
//							glTexSubImage2D2(GL_TEXTURE_2D, level, dynamicTexture.tex % 16 * s, dynamicTexture.tex / 16 * s, s, s, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
//					}
//				}
//			}
//	}
//
//	for (int i = 0; i < dynamicTextures.size(); i++) {
//		DynamicTexture dynamicTexture = dynamicTextures.get(i);
//
//		if (dynamicTexture.copyTo > 0) {
//			pixels.clear();
//			pixels.put(dynamicTexture.pixels);
//			pixels.position(0).limit(dynamicTexture.pixels.length);
//			glBindTexture2(GL_TEXTURE_2D, dynamicTexture.copyTo);
//			glTexSubImage2D2(GL_TEXTURE_2D, 0, 0, 0, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
//			if (MIPMAP) {
//				for (int level = 1; level <= 4; level++) {
//					int os = 16 >> (level - 1);
//					int s = 16 >> level;
//
//					for (int x = 0; x < s; x++)
//						for (int y = 0; y < s; y++) {
//							int c0 = pixels.getInt(((x * 2 + 0) + (y * 2 + 0) * os) * 4);
//							int c1 = pixels.getInt(((x * 2 + 1) + (y * 2 + 0) * os) * 4);
//							int c2 = pixels.getInt(((x * 2 + 1) + (y * 2 + 1) * os) * 4);
//							int c3 = pixels.getInt(((x * 2 + 0) + (y * 2 + 1) * os) * 4);
//							int col = smoothBlend(smoothBlend(c0, c1), smoothBlend(c2, c3));
//							pixels.putInt((x + y * s) * 4, col);
//						}
//						glTexSubImage2D2(GL_TEXTURE_2D, level, 0, 0, s, s, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
//				}
//			}
//		}
//	}
//}
//  void releaseTexture(int id) {
//      loadedImages.erase(id);
//      glDeleteTextures(1, (const GLuint*)&id);
//  }

