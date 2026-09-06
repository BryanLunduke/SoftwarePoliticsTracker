#include "ModelPart.h"
#include "Cube.h"
#include "../Model.h"
#include "../../renderer/Tesselator.h"
#include "../../../util/Mth.h"


ModelPart::ModelPart( const std::string& id )
:	id(id),
	model(NULL),
	xTexOffs(0),
	yTexOffs(0)
{
	_init();
}

ModelPart::ModelPart( int xTexOffs /*= 0*/, int yTexOffs /*= 0*/ )
:	xTexOffs(xTexOffs),
	yTexOffs(yTexOffs),
	model(NULL)
{
	_init();
}

ModelPart::ModelPart( Model* model, int xTexOffs /*= 0*/, int yTexOffs /*= 0*/ )
:	model(model),
	xTexOffs(xTexOffs),
	yTexOffs(yTexOffs)
{
	_init();
}

ModelPart::~ModelPart() {
	clear();
}

void ModelPart::_init() {
	x = y = z = 0;
	xRot = yRot = zRot = 0;
	list  = 0;
	mirror = false;
	visible = true;
	neverRender = false;
	compiled = false;
	xTexSize = 64;
	yTexSize = 32;

	vboId = 0;
	glGenBuffers2(1, &vboId);
}

void ModelPart::setModel(Model* model) {
	if (this->model) {
		Util::remove(this->model->cubes, this);
	}
	if (model) {
		model->cubes.push_back(this);
		setTexSize(model->texWidth, model->texHeight);
	}
	this->model = model;
}

void ModelPart::addChild(ModelPart* child) {
	children.push_back(child);
}

ModelPart& ModelPart::addBox(const std::string& id, float x0, float y0, float z0, int w, int h, int d) {
	std::string newid = this->id + "." + id;
	//TexOffs offs = model.getMapTex(id); //@todo @diff
	//texOffs(offs.x, offs.y);
	cubes.push_back((new Cube(this, xTexOffs, yTexOffs, x0, y0, z0, w, h, d, 0))->setId(newid));
	return *this;
}

ModelPart& ModelPart::addBox(float x0, float y0, float z0, int w, int h, int d) {
	cubes.push_back(new Cube(this, xTexOffs, yTexOffs, x0, y0, z0, w, h, d, 0));
	return *this;
}

void ModelPart::addBox(float x0, float y0, float z0, int w, int h, int d, float g) {
	cubes.push_back(new Cube(this, xTexOffs, yTexOffs, x0, y0, z0, w, h, d, g));
}

//This Cube constructor is commented out
//void ModelPart::addTexBox(float x0, float y0, float z0, int w, int h, int d, int tex) {
//	cubes.push_back(new Cube(this, xTexOffs, yTexOffs, x0, y0, z0, w, h, d, tex));
//}


void ModelPart::setPos( float x, float y, float z )
{
	this->x = x;
	this->y = y;
	this->z = z;
}

void ModelPart::render( float scale )
{
	if (neverRender) return;
	if (!visible) return;
	if (!compiled) compile(scale);

	if (xRot != 0 || yRot != 0 || zRot != 0) {
		glPushMatrix2();
		glTranslatef2(x * scale, y * scale, z * scale);

		const float c = Mth::RADDEG;
		if (zRot != 0) glRotatef2(zRot * c, 0.0f, 0.0f, 1.0f);
		if (yRot != 0) glRotatef2(yRot * c, 0.0f, 1.0f, 0.0f);
		if (xRot != 0) glRotatef2(xRot * c, 1.0f, 0.0f, 0.0f);

		//LOGI("A");
		draw();
		if (!children.empty()) {
            for (unsigned int i = 0; i < children.size(); i++) {
                children[i]->render(scale);
            }
		}

		glPopMatrix2();
	} else if (x != 0 || y != 0 || z != 0) {
		glTranslatef2(x * scale, y * scale, z * scale);
		//LOGI("B");
		draw();
		if (!children.empty()) {
			for (unsigned int i = 0; i < children.size(); i++) {
				children[i]->render(scale);
			}
		}

		glTranslatef2(-x * scale, -y * scale, -z * scale);
	} else {
		//LOGI("C");
		draw();
		if (!children.empty()) {
			for (unsigned int i = 0; i < children.size(); i++) {
				children[i]->render(scale);
			}
		}
	}
}


void ModelPart::renderRollable( float scale )
{
	if (neverRender) return;
	if (!visible) return;
	if (!compiled) compile(scale);

	glPushMatrix2();
	glTranslatef2(x * scale, y * scale, z * scale);

	const float c = Mth::RADDEG;
	if (yRot != 0) glRotatef2(yRot * c, 0.0f, 1.0f, 0.0f);
	if (xRot != 0) glRotatef2(xRot * c, 1.0f, 0.0f, 0.0f);
	if (zRot != 0) glRotatef2(zRot * c, 0.0f, 0.0f, 1.0f);

	draw();
	glPopMatrix2();
}


void ModelPart::translateTo( float scale )
{
	if (neverRender) return;
	if (!visible) return;
	if (!compiled) compile(scale);

	if (xRot != 0 || yRot != 0 || zRot != 0) {
		const float c = Mth::RADDEG;
		glTranslatef2(x * scale, y * scale, z * scale);
		if (zRot != 0) glRotatef2(zRot * c, 0.0f, 0.0f, 1.0f);
		if (yRot != 0) glRotatef2(yRot * c, 0.0f, 1.0f, 0.0f);
		if (xRot != 0) glRotatef2(xRot * c, 1.0f, 0.0f, 0.0f);
	} else if (x != 0 || y != 0 || z != 0) {
		glTranslatef2(x * scale, y * scale, z * scale);
	} else {
	}
}


void ModelPart::compile( float scale )
{
#ifndef OPENGL_ES
	list = glGenLists(1);
	// FIX NORMAL BUG HERE
	glNewList(list, GL_COMPILE);
#endif
	Tesselator& t = Tesselator::instance;
	t.begin();
	t.color(255, 255, 255, 255);
	for (int i = 0; i < 6; i++) {
		for (unsigned int i = 0; i < cubes.size(); ++i)
			cubes[i]->compile(t, scale);
	}
	t.end(true, vboId);
#ifndef OPENGL_ES
	glEndList();
#endif
	compiled = true;
}

void ModelPart::draw()
{
#ifdef OPENGL_ES
	drawArrayVT_NoState(vboId, cubes.size() * 2 * 3 * 6, 24);
#else
	glCallList(list);
#endif
}

ModelPart& ModelPart::setTexSize(int xs, int ys) {
    xTexSize = (float)xs;
    yTexSize = (float)ys;
    return *this;
}

ModelPart& ModelPart::texOffs(int xTexOffs, int yTexOffs) {
	this->xTexOffs = xTexOffs;
	this->yTexOffs = yTexOffs;
	return *this;
}

void ModelPart::mimic(const ModelPart* o) {
    x = o->x;
    y = o->y;
    z = o->z;

    xRot = o->xRot;
    yRot = o->yRot;
    zRot = o->zRot;
}

void ModelPart::renderHorrible( float scale )
{
	if (neverRender) {
		return;
	}
	if (!visible) {
		return;
	}
	//if (!compiled) compile(scale);

	if (xRot != 0 || yRot != 0 || zRot != 0) {
		glPushMatrix2();
		glTranslatef2(x * scale, y * scale, z * scale);
		if (zRot != 0) glRotatef2(zRot * Mth::RADDEG, 0.0f, 0.0f, 1.0f);
		if (yRot != 0) glRotatef2(yRot * Mth::RADDEG, 0.0f, 1.0f, 0.0f);
		if (xRot != 0) glRotatef2(xRot * Mth::RADDEG, 1.0f, 0.0f, 0.0f);

		drawSlow(scale);
		glPopMatrix2();
	} else if (x != 0 || y != 0 || z != 0) {
		glTranslatef2(x * scale, y * scale, z * scale);
		drawSlow(scale);
		glTranslatef2(-x * scale, -y * scale, -z * scale);
	} else {
		drawSlow(scale);
	}
}

void ModelPart::drawSlow( float scale )
{
	Tesselator& t = Tesselator::instance;
	t.begin();
	for (int j = 0; j < (int)cubes.size(); ++j) {
		Cube* c = cubes[j];
		for (int i = 0; i < 6; i++) {
			c->polygons[i].render(t, scale, vboId);
		}
	}
	t.draw();
}

void ModelPart::clear()
{
	for (unsigned int i = 0; i < cubes.size(); ++i)
		delete cubes[i];
	cubes.clear();

	setModel(NULL);
}

ModelPart& ModelPart::operator=( const ModelPart& rhs )
{
	clear();

	if (!id.empty() || !rhs.id.empty()) {
		id = rhs.id;
	}
	xTexOffs = rhs.xTexOffs;
	yTexOffs = rhs.yTexOffs;

	compiled = false;
	mirror = rhs.mirror;

	setModel(rhs.model);
	cubes.assign(rhs.cubes.begin(), rhs.cubes.end());

	mimic(&rhs);

	return *this;
}
