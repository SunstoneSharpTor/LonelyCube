#pragma once
	
//#include <GL/glew.h>
#include <glad/glad.h>

#include "vertexArray.h"
#include "indexBuffer.h"
#include "shader.h"

void GLClearError();

void GLPrintErrors();

class Renderer {
public:
	void clear() const;

	void draw(const VertexArray& va, const IndexBuffer& ib, const Shader& s) const;

	void drawWireframe(const VertexArray& va, const IndexBuffer& ib, const Shader& s) const;

	void setOpenGlOptions() const;
};