#ifndef NET_MINECRAFT_CLIENT_MODEL_GEOM__ModelPart_H__
#define NET_MINECRAFT_CLIENT_MODEL_GEOM__ModelPart_H__

#include "../../renderer/gles.h"

class Model;
class Cube;

class ModelPart
{
public:
	ModelPart(const std::string& id);
	ModelPart(int xTexOffs = 0, int yTexOffs = 0);
	ModelPart(Model* model, int xTexOffs = 0, int yTexOffs = 0);
	~ModelPart();

	void _init();
	void clear();//const ModelPart& rhs);
	ModelPart& operator=(const ModelPart& rhs);

	void setModel(Model* model);

	void setPos( float x, float y, float z );
	void translateTo( float scale );

	ModelPart& setTexSize(int xs, int ys);
	ModelPart& texOffs(int xTexOffs, int yTexOffs);

	void mimic(const ModelPart* o);

	// Render normally
	void render( float scale );
	void renderRollable( float scale );
	void draw();
	// Bad, immediate version... //@fix @todo: remove this
	void renderHorrible(float scale);
	void drawSlow( float scale );

	void onGraphicsReset() { compiled = false; }
	void compile( float scale );

	void addChild(ModelPart* child);
	ModelPart& addBox(const std::string& id, float x0, float y0, float z0, int w, int h, int d);
	ModelPart& addBox(float x0, float y0, float z0, int w, int h, int d);
	void addBox(float x0, float y0, float z0, int w, int h, int d, float g);
	void addTexBox(float x0, float y0, float z0, int w, int h, int d, int tex);

	float x, y, z;
	float xRot, yRot, zRot;

	bool mirror;
    bool visible;

	std::vector<Cube*> cubes;
	std::vector<ModelPart*> children;

	std::string id;

	float xTexSize;
	float yTexSize;

private:
	int xTexOffs, yTexOffs;

	bool neverRender;

	bool compiled;
	int list;
	GLuint vboId;

	Model* model;
};

#endif /*NET_MINECRAFT_CLIENT_MODEL_GEOM__ModelPart_H__*/
