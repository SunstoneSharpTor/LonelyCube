/*
  Lonely Cube, a voxel game
  Copyright (C) g 2024-2025 Bertie Cartwright

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

#include "client/graphics/glRenderer.h"

#include "core/log.h"

namespace lonelycube::client {

void GLClearError() {
    while (glGetError() != GL_NO_ERROR);
}

void GLPrintErrors() {
    while (GLenum error = glGetError()) {
        LOG("OpenGL error: " + std::to_string(error));
    }
}

void GlRenderer::draw(const VertexArray& va, uint32_t count, const Shader& s) const {
    s.bind();
    va.bind();
    glDrawArrays(GL_TRIANGLES, 0, count);
}

void GlRenderer::draw(const VertexArray& va, const IndexBuffer& ib, const Shader& s) const {
    s.bind();
    va.bind();
    ib.bind();
    glDrawElements(GL_TRIANGLES, ib.getCount(), GL_UNSIGNED_INT, nullptr);
}

void GlRenderer::drawWireframe(const VertexArray& va, const IndexBuffer& ib, const Shader& s) const {
    s.bind();
    va.bind();
    ib.bind();
    glDrawElements(GL_LINE_STRIP, ib.getCount(), GL_UNSIGNED_INT, nullptr);
}

void GlRenderer::clear() const {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GlRenderer::setOpenGlOptions() const {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

}  // namespace lonelycube::client
