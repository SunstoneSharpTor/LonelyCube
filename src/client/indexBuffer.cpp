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

#include "client/indexBuffer.h"
#include "client/renderer.h"

namespace client {

IndexBuffer::IndexBuffer() {
    m_rendererID = 0;
    m_count = 0;
}

IndexBuffer::IndexBuffer(const unsigned int* data, unsigned int count) : m_count(count) {
    glGenBuffers(1, &m_rendererID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_count * sizeof(GLuint), data, GL_DYNAMIC_DRAW);
}

IndexBuffer::~IndexBuffer() {
    if (m_rendererID != 0) {
        glDeleteBuffers(1, &m_rendererID);
    }
}

void IndexBuffer::bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);
}
void IndexBuffer::unbind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

}  // namespace client