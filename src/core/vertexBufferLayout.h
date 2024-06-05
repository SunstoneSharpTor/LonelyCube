#pragma once

#include <glad/glad.h>
#include <vector>

struct VertexBufferElement {
	unsigned int type;
	unsigned int count;
	unsigned char normalised;

	static unsigned int getSizeOfType(unsigned int type) {
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
	unsigned int m_stride;
public:
	VertexBufferLayout() : m_stride(0) {}

	template<typename T>
	inline void push(unsigned int count) {
		//static_assert(false);
	}

	inline const std::vector<VertexBufferElement> getElements() const { return m_elements; }
	inline unsigned int getStride() const { return m_stride; }
};

template<>
inline void VertexBufferLayout::push<float>(unsigned int count) {
	m_elements.push_back({ GL_FLOAT, count, GL_FALSE });
	m_stride += VertexBufferElement::getSizeOfType(GL_FLOAT) * count;
}

template<>
inline void VertexBufferLayout::push<unsigned int>(unsigned int count) {
	m_elements.push_back({ GL_UNSIGNED_INT, count, GL_FALSE });
	m_stride += VertexBufferElement::getSizeOfType(GL_UNSIGNED_INT)* count;
}

template<>
inline void VertexBufferLayout::push<unsigned char>(unsigned int count) {
	m_elements.push_back({ GL_UNSIGNED_BYTE, count, GL_TRUE });
	m_stride += VertexBufferElement::getSizeOfType(GL_UNSIGNED_BYTE) * count;
}