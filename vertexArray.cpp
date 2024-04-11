#include "vertexArray.h"
#include "renderer.h"

vertexArray::vertexArray(bool empty) {
	if (empty) {
		m_rendererID = 0;
	}
}
vertexArray::vertexArray() {
	glGenVertexArrays(1, &m_rendererID);
}
vertexArray::~vertexArray() {
	if (m_rendererID != 0) {
		glDeleteVertexArrays(1, &m_rendererID);
	}
}

void vertexArray::addBuffer(const vertexBuffer& vb, const vertexBufferLayout& layout) {
	bind();
	vb.bind();
	const auto& elements = layout.getElements();
	unsigned int offset = 0;
	for (unsigned int i = 0; i < elements.size(); i++) {
		const auto& element = elements[i];
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, element.count, element.type, element.normalised, layout.getStride(), (const void*)offset);
		offset += element.count * vertexBufferElement::getSizeOfType(element.type);
	}
}

void vertexArray::bind() const {
	glBindVertexArray(m_rendererID);
}

void vertexArray::unbind() const {
	glBindVertexArray(0);
}