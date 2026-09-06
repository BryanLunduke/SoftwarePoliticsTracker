#ifndef NET_MINECRAFT_CLIENT_GUI__NinePatch_H__
#define NET_MINECRAFT_CLIENT_GUI__NinePatch_H__

#include "ImageButton.h"
#include "../../renderer/TextureData.h"
#include "../../renderer/Textures.h"
#include "../../renderer/Tesselator.h"
#include "../../Minecraft.h"

class Tesselator;

class NinePatchDescription {
public:
	NinePatchDescription& transformUVForImage(const TextureData& d);
	NinePatchDescription& transformUVForImageSize(int w, int h);

    float u0, u1, u2, u3;
    float v0, v1, v2, v3;
	float w, e, n, s;

    static NinePatchDescription createSymmetrical(int texWidth, int texHeight, const IntRectangle& src, int xCutAt, int yCutAt);
private:
	NinePatchDescription(	float x, float y, float x1, float x2, float x3, float y1, float y2, float y3,
							float w, float e, float n, float s);

	int imgW;
    int imgH;
};

class NinePatchLayer: public GuiElement
{
	struct CachedQuad;
public:
    NinePatchLayer(const NinePatchDescription& desc, const std::string& imageName, Textures* textures, float w = 32, float h = 32);
	virtual ~NinePatchLayer() {};
	void setSize(float w, float h);
    
    void draw(Tesselator& t, float x, float y);

	NinePatchLayer* exclude(int excludeId);
	NinePatchLayer* setExcluded(int exludeBits);

	float getWidth()  { return w; }
	float getHeight() { return h; }

private:
	void buildQuad(int qid);
	void getPatchInfo(int xc, int yc, float& x0, float& x1, float& y0, float& y1);

	void d(Tesselator& t, const CachedQuad& q);

	float w, h;
    NinePatchDescription desc;
    std::string imageName;
    Textures* textures;
	int excluded;

	typedef struct CachedQuad {
		float x0, x1, y0, y1, z;
		float u0, u1, v0, v1;
	} CachedQuad;
	CachedQuad quads[9];
};

class NinePatchFactory {
public:
	NinePatchFactory(Textures* textures, const std::string& imageName );

	NinePatchLayer* createSymmetrical(const IntRectangle& src, int xCutAt, int yCutAt, float w = 32.0f, float h = 32.0f);

private:
	Textures* textures;
	std::string imageName;
	int width;
	int height;
};

#endif /*NET_MINECRAFT_CLIENT_GUI__NinePatch_H__*/
