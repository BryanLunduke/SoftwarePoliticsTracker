#ifndef NET_MINECRAFT_CLIENT_RENDERER__GameRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER__GameRenderer_H__

//package net.minecraft.client.renderer;

#include "gles.h"
#include <cstddef>
#include "../../util/SmoothFloat.h"
#include "../../world/phys/Vec3.h"
#include "../gui/components/ImageButton.h"

class Minecraft;
class Entity;
class ItemInHandRenderer;
class LevelRenderer;
class GameRenderer
{
public:
	GameRenderer(Minecraft* mc_);
	~GameRenderer();

	void pick(float a);

	void updateAllChunks();

	void zoomRegion(float zoom, float xa, float ya);
	void unZoomRegion();
	void setupGuiScreen(bool clearColorBuffer);

	void tick(int nTick, int maxTick);
	void render(float a);
	void renderLevel(float a);
	void renderItemInHand(float a, int eye);

	void onGraphicsReset();

	void setupCamera(float a, int eye);
	void moveCameraToPlayer(float a);

	void setupClearColor(float a);
	float getFov(float a, bool applyEffects);
private:
	void setupFog(int i);

	void tickFov();
	

	void bobHurt(float a);
	void bobView(float a);

	bool updateFreeformPickDirection(float a, Vec3& outDir);
	void prepareAndRenderClouds(LevelRenderer* levelRenderer, float a);

public:
	ItemInHandRenderer* itemInHandRenderer;

private:
	Minecraft* mc;

	float renderDistance;
	int _tick;
	Vec3 pickDirection;

	// smooth camera movement
	SmoothFloat smoothTurnX;
	SmoothFloat smoothTurnY;

	//    // third-person distance etc
	//    SmoothFloat smoothDistance = /*new*/ SmoothFloat();
	//    SmoothFloat smoothRotation = /*new*/ SmoothFloat();
	//    SmoothFloat smoothTilt = /*new*/ SmoothFloat();
	//    SmoothFloat smoothRoll = /*new*/ SmoothFloat();

	float thirdDistance;
	float thirdDistanceO;
	float thirdRotation;
	float thirdRotationO;
	float thirdTilt;
	float thirdTiltO;

	// zoom
	float zoom;
	float zoom_x;
	float zoom_y;

	// fov modification
	float fov, oFov;
	float fovOffset;
	float fovOffsetO;
	float _setupCameraFov;

	// roll modification
	float cameraRoll;
	float cameraRollO;

	float fr;
	float fg;
	float fb;

	float fogBrO, fogBr;

	float _rotX;
	float _rotY;
	float _rotXlast;
	float _rotYlast;
	float _lastTickT;

	void saveMatrices();
	float lastProjMatrix[16];
	float lastModelMatrix[16];

	// Scissor area that Minecraft::screen defines
	bool useScreenScissor;
	IntRectangle screenScissorArea;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__GameRenderer_H__*/
