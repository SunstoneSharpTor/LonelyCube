/*
  Lonely Cube, a voxel game
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

#pragma once

#include <glad/glad.h>

#include "core/pch.h"

#include "shader.h"
#include "vertexArray.h"
#include "vertexBuffer.h"
#include "vertexBufferLayout.h"

namespace client {

template<bool zBuffer>
class FrameBuffer {
private:
    unsigned int m_rendererID;
    unsigned int m_textureColourbuffer;
    unsigned int m_textureDepthBuffer;
    VertexArray m_screenVA;
    std::unique_ptr<VertexBuffer> m_screenVB;
    VertexBufferLayout m_screenVBL;
public:
    FrameBuffer(int* frameSize);
    ~FrameBuffer();
    void bind();
    void unbind();
    void resize(int* frameSize);
    void draw(Shader& shader);
    unsigned int getTextureColourBuffer() {
        return m_textureColourbuffer;
    }
};

template<bool zBuffer>
FrameBuffer<zBuffer>::FrameBuffer(int* frameSize) {
    glGenFramebuffers(1, &m_rendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);
    glGenTextures(1, &m_textureColourbuffer);
    glBindTexture(GL_TEXTURE_2D, m_textureColourbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, frameSize[0], frameSize[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_textureColourbuffer, 0);
    if (zBuffer) {
        glGenTextures(1, &m_textureDepthBuffer);
        glBindTexture(GL_TEXTURE_2D, m_textureDepthBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, frameSize[0], frameSize[1], 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_textureDepthBuffer, 0);
    }
    float screenCoords[24] = { -1.0f, -1.0f, 0.0f, 0.0f,
                                        1.0f,  -1.0f, 1.0f,  0.0f,
                                        -1.0f, 1.0f, 0.0f, 1.0f,
                                       -1.0f, 1.0f, 0.0f, 1.0f,
                                       1.0f,  -1.0f, 1.0f,  0.0f,
                                        1.0f,  1.0f, 1.0f,  1.0f };
    m_screenVB = std::make_unique<VertexBuffer>(screenCoords, 24 * sizeof(float));
    m_screenVBL.push<float>(2);
    m_screenVBL.push<float>(2);
    m_screenVA.addBuffer(*m_screenVB, m_screenVBL);
}

template<bool zBuffer>
FrameBuffer<zBuffer>::~FrameBuffer() {
    glDeleteFramebuffers(1, &m_rendererID);
}

template<bool zBuffer>
void FrameBuffer<zBuffer>::resize(int* frameSize) {
    glBindTexture(GL_TEXTURE_2D, m_textureColourbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, frameSize[0], frameSize[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    if(zBuffer) {
        glBindTexture(GL_TEXTURE_2D, m_textureDepthBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, frameSize[0], frameSize[1], 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    }
}

template<bool zBuffer>
void FrameBuffer<zBuffer>::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);
}

template<bool zBuffer>
void FrameBuffer<zBuffer>::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

template<bool zBuffer>
void FrameBuffer<zBuffer>::draw(Shader& shader) {
    shader.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureColourbuffer);
    m_screenVA.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

}  // namespace client