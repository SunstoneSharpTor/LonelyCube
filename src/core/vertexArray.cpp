#include "vertexArray.h"
#include "renderer.h"

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