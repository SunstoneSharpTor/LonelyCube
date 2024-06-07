/*
  Copyright (C) 2024 Bertie Cartwright

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <glad/glad.h>

#include "vertexArray.h"
#include "indexBuffer.h"
#include "shader.h"

namespace client {

void GLClearError();

void GLPrintErrors();

class Renderer {
public:
	void clear() const;

	void draw(const VertexArray& va, const IndexBuffer& ib, const Shader& s) const;

	void drawWireframe(const VertexArray& va, const IndexBuffer& ib, const Shader& s) const;

	void setOpenGlOptions() const;
};

}  // namespace client