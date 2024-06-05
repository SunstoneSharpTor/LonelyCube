#include "renderer.h"

#include <iostream>

void GLClearError() {
    while (glGetError() != GL_NO_ERROR);
}

void GLPrintErrors() {
    while (GLenum error = glGetError()) {
        std::cout << "OpenGL error: " << error << std::endl;
    }
}

void Renderer::draw(const VertexArray& va, const IndexBuffer& ib, const Shader& s) const {
    s.bind();
    va.bind();
    ib.bind();
    glDrawElements(GL_TRIANGLES, ib.getCount(), GL_UNSIGNED_INT, nullptr);
}

void Renderer::drawWireframe(const VertexArray& va, const IndexBuffer& ib, const Shader& s) const {
    s.bind();
    va.bind();
    ib.bind();
    glDrawElements(GL_LINE_STRIP, ib.getCount(), GL_UNSIGNED_INT, nullptr);
}

void Renderer::clear() const {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::setOpenGlOptions() const {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}