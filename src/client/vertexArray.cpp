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

#include "client/vertexArray.h"
#include "client/renderer.h"

namespace client {

VertexArray::VertexArray(bool empty) {
	if (empty) {
		m_rendererID = 0;
	}
}
VertexArray::VertexArray() {
	glGenVertexArrays(1, &m_rendererID);
}
VertexArray::~VertexArray() {
	if (m_rendererID != 0) {
		glDeleteVertexArrays(1, &m_rendererID);
	}
}

void VertexArray::addBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout) {
	bind();
	vb.bind();
	const auto& elements = layout.getElements();
	unsigned int offset = 0;
	for (unsigned int i = 0; i < elements.size(); i++) {
		const auto& element = elements[i];
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, element.count, element.type, element.normalised, layout.getStride(), (const void*)offset);
		offset += element.count * VertexBufferElement::getSizeOfType(element.type);
	}
}

void VertexArray::bind() const {
	glBindVertexArray(m_rendererID);
}

void VertexArray::unbind() const {
	glBindVertexArray(0);
}

}  // namespace client