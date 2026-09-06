#ifndef NET_MINECRAFT_CLIENT_RENDERER__VertexDecl_H__
#define NET_MINECRAFT_CLIENT_RENDERER__VertexDecl_H__

typedef struct VertexDeclPTC
{
	GLfloat x, y, z;
	GLfloat u, v;
	GLuint color;

} VertexDeclPTC;

typedef struct VertexDeclPTCN
{
	GLfloat x, y, z;
	GLfloat u, v;
	GLuint color;
	GLuint normal;

} VertexDeclPTCN;

#endif /*#ifndef NET_MINECRAFT_CLIENT_RENDERER__VertexDecl_H__ */
