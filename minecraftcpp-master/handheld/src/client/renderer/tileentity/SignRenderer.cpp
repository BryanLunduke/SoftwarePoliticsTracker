#include "SignRenderer.h"
#include "../../../world/level/tile/Tile.h"
#include "../../../world/level/tile/entity/SignTileEntity.h"

void SignRenderer::render( TileEntity* te, float x, float y, float z, float a )
{
    SignTileEntity* sign = (SignTileEntity*) te;
    Tile* tile = sign->getTile();

    glPushMatrix();
    float size = 16 / 24.0f;
    if (tile == Tile::sign) {
        glTranslatef(x + 0.5f, y + 0.75f * size, z + 0.5f);
        float rot = sign->getData() * 360 / 16.0f;
        glRotatef(-rot, 0, 1, 0);
        signModel.cube2.visible = true;
    } else {
        int face = sign->getData();
        float rot = 0;

        if (face == 2) rot = 180;
        if (face == 4) rot = 90;
        if (face == 5) rot = -90;

        glTranslatef(x + 0.5f, y + 0.75f * size, z + 0.5f);
        glRotatef(-rot, 0, 1, 0);
        glTranslatef(0, -5 / 16.0f, -7 / 16.0f);

        signModel.cube2.visible = false;
    }

    bindTexture("item/sign.png");

    glPushMatrix();
    glScalef(size, -size, -size);
    signModel.render();
    glPopMatrix();
    Font* font = getFont();

    float s = 1 / 60.0f * size;
    glTranslatef(0, 0.5f * size, 0.07f * size);
    glScalef(s, -s, s);
    glNormal3f(0, 0, -1 * s);
    glDepthMask(false);

    int col = 0;
	float yy = (float)(SignTileEntity::NUM_LINES * -5);
    for (int i = 0; i < SignTileEntity::NUM_LINES; i++) {
        std::string& msg = sign->messages[i];
        if (i == sign->selectedLine) {
            std::string s = "> " + msg + " <";
            font->draw(s, (float)-font->width(s) / 2, yy, col);
        } else {
            font->draw(msg, (float)-font->width(msg) / 2, yy, col);
        }
		yy += 10;
    }
    glDepthMask(true);
    glColor4f(1, 1, 1, 1);
    glPopMatrix();
}

void SignRenderer::onGraphicsReset() {
	signModel.onGraphicsReset();
}
