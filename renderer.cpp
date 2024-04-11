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

void renderer::draw(const vertexArray& va, const indexBuffer& ib, const shader& s) const {
    s.bind();
    va.bind();
    ib.bind();
    glDrawElements(GL_TRIANGLES, ib.getCount(), GL_UNSIGNED_INT, nullptr);
}

void renderer::drawWireframe(const vertexArray& va, const indexBuffer& ib, const shader& s) const {
    s.bind();
    va.bind();
    ib.bind();
    glDrawElements(GL_LINE_STRIP, ib.getCount(), GL_UNSIGNED_INT, nullptr);
}

void renderer::clear() const {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderer::setOpenGlOptions() const {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}