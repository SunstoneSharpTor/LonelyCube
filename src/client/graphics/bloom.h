/*
  Lonely Cube, a voxel game
  Copyright (C) 2024-2025 Bertie Cartwright

  Lonely Cube is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Lonely Cube is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "core/pch.h"

#include "glm/glm.hpp"

namespace lonelycube::client {

struct BloomMip {
    glm::vec2 size;
    glm::ivec2 intSize;
    uint32_t texture;
};

class Bloom {
public:
private:
    ComputeShader& m_downsampleShader;
    ComputeShader& m_upsampleShader;
    ComputeShader& m_blitShader;
    std::vector<BloomMip> m_mipChain;
    BloomMip m_srcTexture;

    void createMips(glm::ivec2 srcTextureSize);
    void deleteMips();
    void renderDownsamples();
    void renderUpsamples(float filterRadius);
public:
    Bloom(uint32_t srcTexture, uint32_t windowSize[2],
        ComputeShader& downsampleShader, ComputeShader& upsampleShader, ComputeShader& blitShader);
    ~Bloom();
    void render(float filterRadius, float strength);
    void resize(uint32_t windowSize[2]);
    BloomMip getSmallestMip() {
        return m_mipChain[m_mipChain.size() - 1];
    }
};

}  // namespace lonelycube::client
