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

#ifndef GLES3

#include "client/graphics/bloom.h"

#include "glad/glad.h"

#include "core/pch.h"

namespace client {

Bloom::Bloom(uint32_t srcTexture, uint32_t windowSize[2], ComputeShader& downsampleShader,
    ComputeShader& upsampleShader, ComputeShader& blitShader) :
    m_downsampleShader(downsampleShader), m_upsampleShader(upsampleShader),
    m_blitShader(blitShader) {
    glm::vec2 mipSize((float)windowSize[0], (float)windowSize[1]);
    glm::ivec2 mipIntSize((int)windowSize[0], (int)windowSize[1]);

    m_srcTexture.size = mipSize;
    m_srcTexture.intSize = mipIntSize;
    m_srcTexture.texture = srcTexture;

    createMips(mipIntSize);
}

Bloom::~Bloom() {
    deleteMips();
}

void Bloom::renderDownsamples() {
    m_downsampleShader.bind();

    // Bind srcTexture (HDR color buffer) as initial texture input
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_srcTexture.texture);

    // Progressively downsample through the mip chain
    for (int i = 0; i < m_mipChain.size(); i++) {
        const BloomMip& mip = m_mipChain[i];
        glBindImageTexture(0, mip.texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        glDispatchCompute((uint32_t)((mip.intSize.x + 7) / 8),
            (uint32_t)((mip.intSize.y + 7) / 8), 1);
        // Make sure writing to image has finished before read
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // Set current mip as texture input for next iteration
        glBindTexture(GL_TEXTURE_2D, mip.texture);
    }
}

void Bloom::renderUpsamples(float filterRadius) {
    m_upsampleShader.bind();
    m_upsampleShader.setUniform1f("filterRadius", filterRadius);

    for (int i = m_mipChain.size() - 1; i > 0; i--) {
        const BloomMip& mip = m_mipChain[i];
        const BloomMip& nextMip = m_mipChain[i-1];

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mip.texture);
        glBindImageTexture(0, nextMip.texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        glDispatchCompute((uint32_t)((nextMip.intSize.x + 7) / 8),
            (uint32_t)((nextMip.intSize.y + 7) / 8), 1);
        // Make sure writing to image has finished before read
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}

void Bloom::render(float filterRadius, float strength) {
    renderDownsamples();
    renderUpsamples(filterRadius);

    //glClear(GL_COLOR_BUFFER_BIT);  // Use to test the bloom image before it's composited

    m_blitShader.bind();
    m_blitShader.setUniform1f("strength", strength);
    m_upsampleShader.setUniform1f("filterRadius", filterRadius);

    const BloomMip& mip = m_mipChain[0];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mip.texture);
    glBindImageTexture(0, m_srcTexture.texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    glDispatchCompute((uint32_t)((m_srcTexture.intSize.x + 7) / 8),
        (uint32_t)((m_srcTexture.intSize.y + 7) / 8), 1);
    // Make sure writing to image has finished before read
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void Bloom::createMips(glm::ivec2 srcTextureSize) {
    glm::ivec2 mipIntSize = srcTextureSize;
    glm::vec2 mipSize = srcTextureSize;

    while (mipIntSize.x > 1 && mipIntSize.y > 1) {
        BloomMip mip;

        mipSize *= 0.5f;
        mipIntSize /= 2;
        mip.size = mipSize;
        mip.intSize = mipIntSize;

        glGenTextures(1, &mip.texture);
        glBindTexture(GL_TEXTURE_2D, mip.texture);
        // we are downscaling an HDR color buffer, so we need a float texture format
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)mipSize.x, (int)mipSize.y, 0,
            GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        GLfloat borderColour[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glTextureParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);

        m_mipChain.emplace_back(mip);
    }
}

void Bloom::deleteMips() {
    for (int i = 0; i < m_mipChain.size(); i++) {
        glDeleteTextures(1, &m_mipChain[i].texture);
    }
    m_mipChain.clear();
}

void Bloom::resize(uint32_t windowSize[2]) {
    deleteMips();
    glm::vec2 mipSize((float)windowSize[0], (float)windowSize[1]);
    glm::ivec2 mipIntSize((int)windowSize[0], (int)windowSize[1]);
    m_srcTexture.size = mipSize;
    m_srcTexture.intSize = mipIntSize;
    createMips(m_srcTexture.intSize);
}

}  // namespace client

#endif
