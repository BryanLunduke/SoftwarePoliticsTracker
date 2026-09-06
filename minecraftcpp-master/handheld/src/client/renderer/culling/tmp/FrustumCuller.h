#ifndef NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumCuller_H__
#define NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumCuller_H__

//package net.minecraft.client.renderer.culling;

#include "Culler.h"
#include "../../../world/phys/AABB.h"

class FrustumCuller: public Culler
{
    float xOff, yOff, zOff;
	float frustum[6][4];

public:    
	FrustumCuller() {
		ExtractFrustum();
	}

	void prepare(float xOff, float yOff, float zOff) {
        this->xOff = xOff;
        this->yOff = yOff;
        this->zOff = zOff;
	}

    //bool cubeFullyInFrustum(float x0, float y0, float z0, float x1, float y1, float z1) {
    //    return frustum.cubeFullyInFrustum(x0 - xOff, y0 - yOff, z0 - zOff, x1 - xOff, y1 - yOff, z1 - zOff);
    //}

	bool cubeFullyInFrustum( float x0, float y0, float z0, float x1, float y1, float z1 ) {
		int c2 = 0;
		for( int p = 0; p < 6; p++ ) {
			int c = 0;
			if( frustum[p][0] * (x0) + frustum[p][1] * (y0) + frustum[p][2]    * (z0) + frustum[p][3] > 0 ) c++;
			if( frustum[p][0] * (x1) + frustum[p][1] * (y0) + frustum[p][2]    * (z0) + frustum[p][3] > 0 ) c++;
			if( frustum[p][0] * (x0) + frustum[p][1] * (y1) + frustum[p][2]    * (z0) + frustum[p][3] > 0 ) c++;
			if( frustum[p][0] * (x1) + frustum[p][1] * (y1) + frustum[p][2]    * (z0) + frustum[p][3] > 0 ) c++;
			if( frustum[p][0] * (x0) + frustum[p][1] * (y0) + frustum[p][2]    * (z1) + frustum[p][3] > 0 ) c++;
			if( frustum[p][0] * (x1) + frustum[p][1] * (y0) + frustum[p][2]    * (z1) + frustum[p][3] > 0 ) c++;
			if( frustum[p][0] * (x0) + frustum[p][1] * (y1) + frustum[p][2]    * (z1) + frustum[p][3] > 0 ) c++;
			if( frustum[p][0] * (x1) + frustum[p][1] * (y1) + frustum[p][2]    * (z1) + frustum[p][3] > 0 ) c++;
			if( c == 0 )
				return false; // 0
			if( c == 8 )
				c2++;
		}
		return c2 == 6; //(c2 == 6) ? 2 : 1;
	}

    bool isVisible(const AABB& bb) {
		printf("cube is : %s\n", bb.toString().c_str());
		return cubeInFrustum(bb.x0, bb.y0, bb.z0, bb.x1, bb.y1, bb.z1);
	}

    bool cubeInFrustum(float x0, float y0, float z0, float x1, float y1, float z1) {
		for(int p = 0; p < 6; p++ ) {
			if( frustum[p][0] * (x0) + frustum[p][1] * (y0) + frustum[p][2]    * (z0) + frustum[p][3] > 0 ) continue;
			if( frustum[p][0] * (x1) + frustum[p][1] * (y0) + frustum[p][2]    * (z0) + frustum[p][3] > 0 ) continue;
			if( frustum[p][0] * (x0) + frustum[p][1] * (y1) + frustum[p][2]    * (z0) + frustum[p][3] > 0 ) continue;
			if( frustum[p][0] * (x1) + frustum[p][1] * (y1) + frustum[p][2]    * (z0) + frustum[p][3] > 0 ) continue;
			if( frustum[p][0] * (x0) + frustum[p][1] * (y0) + frustum[p][2]    * (z1) + frustum[p][3] > 0 ) continue;
			if( frustum[p][0] * (x1) + frustum[p][1] * (y0) + frustum[p][2]    * (z1) + frustum[p][3] > 0 ) continue;
			if( frustum[p][0] * (x0) + frustum[p][1] * (y1) + frustum[p][2]    * (z1) + frustum[p][3] > 0 ) continue;
			if( frustum[p][0] * (x1) + frustum[p][1] * (y1) + frustum[p][2]    * (z1) + frustum[p][3] > 0 ) continue;
			return false;
		}
		return true;
	}

private:
	void ExtractFrustum() {
		float proj[16];
		float modl[16];
		float clip[16];
		float t;
		/* Get the current PROJECTION matrix from OpenGL */
		glGetFloatv( GL_PROJECTION_MATRIX, proj );
		/* Get the current MODELVIEW matrix from OpenGL */
		glGetFloatv( GL_MODELVIEW_MATRIX, modl );
		/* Combine the two matrices (multiply projection by modelview)    */
		clip[ 0] = modl[ 0] * proj[ 0] + modl[ 1] * proj[ 4] + modl[ 2] * proj[ 8] +    modl[ 3] * proj[12];
		clip[ 1] = modl[ 0] * proj[ 1] + modl[ 1] * proj[ 5] + modl[ 2] * proj[ 9] +    modl[ 3] * proj[13];
		clip[ 2] = modl[ 0] * proj[ 2] + modl[ 1] * proj[ 6] + modl[ 2] * proj[10] +    modl[ 3] * proj[14];
		clip[ 3] = modl[ 0] * proj[ 3] + modl[ 1] * proj[ 7] + modl[ 2] * proj[11] +    modl[ 3] * proj[15];
		clip[ 4] = modl[ 4] * proj[ 0] + modl[ 5] * proj[ 4]    + modl[ 6] * proj[ 8] + modl[ 7] * proj[12];
		clip[ 5] = modl[ 4] * proj[ 1] + modl[ 5] * proj[ 5] + modl[ 6] * proj[ 9] +    modl[ 7] * proj[13];
		clip[ 6] = modl[ 4] * proj[ 2] + modl[ 5] * proj[ 6] + modl[ 6] * proj[10] +    modl[ 7] * proj[14];
		clip[ 7] = modl[ 4] * proj[ 3] + modl[ 5] * proj[ 7] + modl[ 6] * proj[11] +    modl[ 7] * proj[15];
		clip[ 8] = modl[ 8] * proj[ 0] + modl[ 9] * proj[ 4]    + modl[10] * proj[ 8] + modl[11] * proj[12];
		clip[ 9] = modl[ 8] * proj[ 1] + modl[ 9] * proj[ 5] + modl[10] * proj[ 9] +    modl[11] * proj[13];
		clip[10] = modl[ 8] * proj[ 2] + modl[ 9] * proj[ 6] + modl[10] * proj[10] +    modl[11] * proj[14];
		clip[11] = modl[ 8] * proj[ 3] + modl[ 9] * proj[ 7] + modl[10] * proj[11] +    modl[11] * proj[15];
		clip[12] = modl[12] * proj[ 0] + modl[13] * proj[ 4]    + modl[14] * proj[ 8] + modl[15] * proj[12];
		clip[13] = modl[12] * proj[ 1] + modl[13] * proj[ 5] + modl[14] * proj[ 9] +    modl[15] * proj[13];
		clip[14] = modl[12] * proj[ 2] + modl[13] * proj[ 6] + modl[14] * proj[10] +    modl[15] * proj[14];
		clip[15] = modl[12] * proj[ 3] + modl[13] * proj[ 7] + modl[14] * proj[11] +    modl[15] * proj[15];
		/* Extract the numbers for the RIGHT plane */
		frustum[0][0] = clip[ 3] - clip[ 0];
		frustum[0][1] = clip[ 7] - clip[ 4];
		frustum[0][2] = clip[11] - clip[ 8];
		frustum[0][3] = clip[15] - clip[12];
		/* Normalize the result */
		t = sqrt( frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2]    * frustum[0][2] );
		frustum[0][0] /= t;
		frustum[0][1] /= t;
		frustum[0][2] /= t;
		frustum[0][3] /= t;
		/* Extract the numbers for the LEFT plane */
		frustum[1][0] = clip[ 3] + clip[ 0];
		frustum[1][1] = clip[ 7] + clip[ 4];
		frustum[1][2] = clip[11] + clip[ 8];
		frustum[1][3] = clip[15] + clip[12];
		/* Normalize the result */
		t = sqrt( frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2]    * frustum[1][2] );
		frustum[1][0] /= t;
		frustum[1][1] /= t;
		frustum[1][2] /= t;
		frustum[1][3] /= t;
		/* Extract the BOTTOM plane */
		frustum[2][0] = clip[ 3] + clip[ 1];
		frustum[2][1] = clip[ 7] + clip[ 5];
		frustum[2][2] = clip[11] + clip[ 9];
		frustum[2][3] = clip[15] + clip[13];
		/* Normalize the result */
		t = sqrt( frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2]    * frustum[2][2] );
		frustum[2][0] /= t;
		frustum[2][1] /= t;
		frustum[2][2] /= t;
		frustum[2][3] /= t;
		/* Extract the TOP plane */
		frustum[3][0] = clip[ 3] - clip[ 1];
		frustum[3][1] = clip[ 7] - clip[ 5];
		frustum[3][2] = clip[11] - clip[ 9];
		frustum[3][3] = clip[15] - clip[13];
		/* Normalize the result */
		t = sqrt( frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2]    * frustum[3][2] );
		frustum[3][0] /= t;
		frustum[3][1] /= t;
		frustum[3][2] /= t;
		frustum[3][3] /= t;
		/* Extract the FAR plane */
		frustum[4][0] = clip[ 3] - clip[ 2];
		frustum[4][1] = clip[ 7] - clip[ 6];
		frustum[4][2] = clip[11] - clip[10];
		frustum[4][3] = clip[15] - clip[14];
		/* Normalize the result */
		t = sqrt( frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2]    * frustum[4][2] );
		frustum[4][0] /= t;
		frustum[4][1] /= t;
		frustum[4][2] /= t;
		frustum[4][3] /= t;
		/* Extract the NEAR plane */
		frustum[5][0] = clip[ 3] + clip[ 2];
		frustum[5][1] = clip[ 7] + clip[ 6];
		frustum[5][2] = clip[11] + clip[10];
		frustum[5][3] = clip[15] + clip[14];
		/* Normalize the result */
		t = sqrt( frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2]    * frustum[5][2] );
		frustum[5][0] /= t;
		frustum[5][1] /= t;
		frustum[5][2] /= t;
		frustum[5][3] /= t;
	}
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumCuller_H__*/
