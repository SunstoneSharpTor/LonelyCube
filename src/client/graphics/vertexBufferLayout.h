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

#pragma once

#include <glad/glad.h>

#include "core/pch.h"

namespace lonelycube::client {

struct VertexBufferElement {
   uint32_t type;
   uint32_t count;
   uint8_t normalised;

   static uint32_t getSizeOfType(uint32_t type) {
      switch (type) {
         case GL_FLOAT:
            return sizeof(GLfloat);
         case GL_UNSIGNED_INT:
            return sizeof(GLuint);
         case GL_UNSIGNED_BYTE:
            return sizeof(GLubyte);
      }
      return 0;
   }
};

class VertexBufferLayout {
private:
   std::vector<VertexBufferElement> m_elements;
   uint32_t m_stride;
public:
   VertexBufferLayout() : m_stride(0) {}

   template<typename T>
   inline void push(uint32_t count) {
      //static_assert(false);
   }

   inline const std::vector<VertexBufferElement> getElements() const { return m_elements; }
   inline uint32_t getStride() const { return m_stride; }
};

template<>
inline void VertexBufferLayout::push<float>(uint32_t count) {
   m_elements.push_back({ GL_FLOAT, count, GL_FALSE });
   m_stride += VertexBufferElement::getSizeOfType(GL_FLOAT) * count;
}

template<>
inline void VertexBufferLayout::push<uint32_t>(uint32_t count) {
   m_elements.push_back({ GL_UNSIGNED_INT, count, GL_FALSE });
   m_stride += VertexBufferElement::getSizeOfType(GL_UNSIGNED_INT)* count;
}

template<>
inline void VertexBufferLayout::push<uint8_t>(uint32_t count) {
   m_elements.push_back({ GL_UNSIGNED_BYTE, count, GL_TRUE });
   m_stride += VertexBufferElement::getSizeOfType(GL_UNSIGNED_BYTE) * count;
}

}  // namespace lonelycube::client
