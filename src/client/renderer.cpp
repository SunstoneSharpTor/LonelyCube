/*
  Lonely Cube, a voxel game
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

#include "client/renderer.h"

#include "core/pch.h"

namespace client {

void GLClearError() {
    while (glGetError() != GL_NO_ERROR);
}

void GLPrintErrors() {
    while (GLenum error = glGetError()) {
        std::cout << "OpenGL error: " << error << std::endl;
    }
}

void Renderer::draw(const VertexArray& va, const IndexBuffer& ib, const Shader& s) const {
    s.bind();
    va.bind();
    ib.bind();
    glDrawElements(GL_TRIANGLES, ib.getCount(), GL_UNSIGNED_INT, nullptr);
}

void Renderer::drawWireframe(const VertexArray& va, const IndexBuffer& ib, const Shader& s) const {
    s.bind();
    va.bind();
    ib.bind();
    glDrawElements(GL_LINE_STRIP, ib.getCount(), GL_UNSIGNED_INT, nullptr);
}

void Renderer::clear() const {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::setOpenGlOptions() const {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

}  // namespace client