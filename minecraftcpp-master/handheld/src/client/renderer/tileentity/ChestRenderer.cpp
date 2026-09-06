#include "ChestRenderer.h"
#include "TileEntityRenderer.h"
#include "../gles.h"
#include "../../model/ChestModel.h"
#include "../../../world/level/tile/entity/ChestTileEntity.h"
#include "../../../world/level/tile/ChestTile.h"
#include "../../../util/Mth.h"


void ChestRenderer::render( TileEntity* entity, float x, float y, float z, float a )
{
	ChestTileEntity* chest = (ChestTileEntity*) entity;
	int data = 0;

	if (chest->level) {
		Tile* tile = chest->getTile();
		data = chest->getData();

		if (tile != NULL && data == 0) {
			((ChestTile*)tile)->recalcLockDir(chest->level, chest->x, chest->y, chest->z);
			data = chest->getData();
		}

		chest->checkNeighbors();
	}
	if (chest->n != NULL || chest->w != NULL) return;

	ChestModel* model;
	//if (chest->e != NULL || chest->s != NULL) {
	//	//model = &largeChestModel;
	//	bindTexture("item/largechest.png");
	//} else
	{
		model = &chestModel;
		bindTexture("item/chest.png");
	}

	glPushMatrix2();
	glColor4f2(1, 1, 1, 1);
	glTranslatef2(x, y + 1, z + 1);
	glScalef2(1, -1, -1);

	glTranslatef2(0.5f, 0.5f, 0.5f);
	GLfloat rot = 0;
	if (data == 2) rot = 180;
	if (data == 3) rot = 0;
	if (data == 4) rot = 90;
	if (data == 5) rot = -90;

	if (data == 2 && chest->e != NULL) {
		glTranslatef2(1, 0, 0);
	}
	if (data == 5 && chest->s != NULL) {
		glTranslatef2(0, 0, -1);
	}
	glRotatef2(rot, 0, 1, 0);
	glTranslatef2(-0.5f, -0.5f, -0.5f);

	float open = chest->oOpenness + (chest->openness - chest->oOpenness) * a;
	if (chest->n != NULL) {
		float open2 = chest->n->oOpenness + (chest->n->openness - chest->n->oOpenness) * a;
		if (open2 > open) open = open2;
	}
	if (chest->w != NULL) {
		float open2 = chest->w->oOpenness + (chest->w->openness - chest->w->oOpenness) * a;
		if (open2 > open) open = open2;
	}

	open = 1 - open;
	open = 1 - open * open * open;

	model->lid.xRot = -(open * Mth::PI / 2);
	model->render();
	glPopMatrix2();
	glColor4f2(1, 1, 1, 1);
}
