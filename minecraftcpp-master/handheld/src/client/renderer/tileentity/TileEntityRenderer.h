#ifndef NET_MINECRAFT_CLIENT_RENDERER_TILEENTITY__TileEntityRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_TILEENTITY__TileEntityRenderer_H__

//package net.minecraft.client.renderer.tileentity;

#include <string>

class Level;
class TileEntity;
class TileEntityRenderDispatcher;
class Font;

class TileEntityRenderer
{
public:
    TileEntityRenderer();
    virtual ~TileEntityRenderer(){}
    
	void init(TileEntityRenderDispatcher* tileEntityRenderDispatcher);
    virtual void render(TileEntity* entity, float x, float y, float z, float a) = 0;

    void bindTexture(const std::string& resourceName);
//     /*protected*/ void bindTexture(const std::string& urlTexture, const std::string& backupTexture) {
//         Textures t = tileEntityRenderDispatcher.textures;
//         if (t != NULL) t.bind(t.loadHttpTexture(urlTexture, backupTexture));
//     }

    Level* getLevel();
	Font* getFont();

	virtual void onGraphicsReset() {}

    virtual void onNewLevel(Level* level) {}
protected:
    TileEntityRenderDispatcher* tileEntityRenderDispatcher;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_TILEENTITY__TileEntityRenderer_H__*/
