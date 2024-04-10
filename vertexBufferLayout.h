#pragma once

#include <GL/glew.h>
#include <vector>

using namespace std;

struct vertexBufferElement {
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

class vertexBufferLayout {
private:
	vector<vertexBufferElement> m_elements;
	unsigned int m_stride;
public:
	vertexBufferLayout() : m_stride(0) {}

	template<typename T>
	inline void push(unsigned int count) {
		//static_assert(false);
	}

	inline const vector<vertexBufferElement> getElements() const { return m_elements; }
	inline unsigned int getStride() const { return m_stride; }
};

template<>
inline void vertexBufferLayout::push<float>(unsigned int count) {
	m_elements.push_back({ GL_FLOAT, count, GL_FALSE });
	m_stride += vertexBufferElement::getSizeOfType(GL_FLOAT) * count;
}

template<>
inline void vertexBufferLayout::push<unsigned int>(unsigned int count) {
	m_elements.push_back({ GL_UNSIGNED_INT, count, GL_FALSE });
	m_stride += vertexBufferElement::getSizeOfType(GL_UNSIGNED_INT)* count;
}

template<>
inline void vertexBufferLayout::push<unsigned char>(unsigned int count) {
	m_elements.push_back({ GL_UNSIGNED_BYTE, count, GL_TRUE });
	m_stride += vertexBufferElement::getSizeOfType(GL_UNSIGNED_BYTE) * count;
}