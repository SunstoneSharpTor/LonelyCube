#include "vertexBuffer.h"
#include "renderer.h"

vertexBuffer::vertexBuffer() {
    m_rendererID = 0;
}

vertexBuffer::vertexBuffer(const void* data, unsigned int size) {
    glGenBuffers(1, &m_rendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
}

vertexBuffer::~vertexBuffer() {
    if (m_rendererID != 0) {
        glDeleteBuffers(1, &m_rendererID);
    }
}

void vertexBuffer::bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
}
void vertexBuffer::unbind() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}