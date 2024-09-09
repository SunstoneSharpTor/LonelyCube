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

#include "client/luminance.h"

#include "glad/glad.h"

#include "core/pch.h"

namespace client {

Luminance::Luminance(unsigned int srcTexture, unsigned int windowSize[2], ComputeShader&
    luminanceShader, ComputeShader& downsampleShader) : m_luminanceShader(luminanceShader),
    m_downsampleShader(downsampleShader) {
    glm::vec2 mipSize((float)windowSize[0], (float)windowSize[1]);
    glm::ivec2 mipIntSize((int)windowSize[0], (int)windowSize[1]);

    m_srcTexture.size = mipSize;
    m_srcTexture.intSize = mipIntSize;
    m_srcTexture.texture = srcTexture;

    createMips(mipIntSize);
}

Luminance::~Luminance() {
    deleteMips();
}

float Luminance::calculate() {
    m_luminanceShader.bind();
    glBindImageTexture(0, m_srcTexture.texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    glBindImageTexture(1, m_mipChain[0].texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R16F);
    glDispatchCompute((unsigned int)((m_mipChain[0].intSize.x + 7) / 8),
        (unsigned int)((m_mipChain[0].intSize.y + 7) / 8), 1);
    // Make sure writing to image has finished before read
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    m_downsampleShader.bind();

    // Progressively downsample through the mip chain
    for (int i = 1; i < m_mipChain.size(); i++) {
        const LuminanceMip& srcMip = m_mipChain[i - 1];
        const LuminanceMip& outputMip = m_mipChain[i];

        glBindImageTexture(0, srcMip.texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R16F);
        glBindImageTexture(1, outputMip.texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R16F);
        glDispatchCompute((unsigned int)((outputMip.intSize.x + 7) / 8),
            (unsigned int)((outputMip.intSize.y + 7) / 8), 1);
        // Make sure writing to image has finished before read
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    float luminance;
    glBindTexture(GL_TEXTURE_2D, m_mipChain[m_mipChain.size() - 1].texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_HALF_FLOAT, &luminance);
    return luminance;
}

void Luminance::createMips(glm::ivec2 srcTextureSize) {
    glm::ivec2 mipIntSize = srcTextureSize;
    glm::vec2 mipSize = srcTextureSize;

    while (mipIntSize.x > 1 || mipIntSize.y > 1) {
        LuminanceMip mip;

        mipSize *= 0.5f;
        mipIntSize.x = std::max(mipIntSize.x / 2, 1);
        mipIntSize.y = std::max(mipIntSize.y / 2, 1);
        mip.size = mipSize;
        mip.intSize = mipIntSize;

        glGenTextures(1, &mip.texture);
        glBindTexture(GL_TEXTURE_2D, mip.texture);
        // we are downscaling an HDR color buffer, so we need a float texture format
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, (int)mipSize.x, (int)mipSize.y, 0,
            GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        GLfloat borderColour[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glTextureParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);

        m_mipChain.emplace_back(mip);
    }
}

void Luminance::deleteMips() {
    for (int i = 0; i < m_mipChain.size(); i++) {
        glDeleteTextures(1, &m_mipChain[i].texture);
    }
    m_mipChain.clear();
}

void Luminance::resize(unsigned int windowSize[2]) {
    deleteMips();
    m_srcTexture.intSize = glm::ivec2(windowSize[0], windowSize[1]);
    m_srcTexture.size = m_srcTexture.intSize;
    createMips(m_srcTexture.intSize);
}

}  // namespace client