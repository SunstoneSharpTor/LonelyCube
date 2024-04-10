#pragma once
	
#include <GL/glew.h>

#include "vertexArray.h"
#include "indexBuffer.h"
#include "shader.h"

void GLClearError();

void GLPrintErrors();

class renderer {
public:
	void clear() const;

	void draw(const vertexArray& va, const indexBuffer& ib, const shader& s) const;

	void drawWireframe(const vertexArray& va, const indexBuffer& ib, const shader& s) const;

	void setOpenGlOptions() const;
};