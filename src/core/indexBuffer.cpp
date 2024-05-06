#include "indexBuffer.h"

#include "indexBuffer.h"
#include "renderer.h"

indexBuffer::indexBuffer() {
    m_rendererID = 0;
    m_count = 0;
}

indexBuffer::indexBuffer(const unsigned int* data, unsigned int count) : m_count(count) {
    glGenBuffers(1, &m_rendererID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_count * sizeof(GLuint), data, GL_DYNAMIC_DRAW);
}

indexBuffer::~indexBuffer() {
    if (m_rendererID != 0) {
        glDeleteBuffers(1, &m_rendererID);
    }
}

void indexBuffer::bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);
}
void indexBuffer::unbind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}