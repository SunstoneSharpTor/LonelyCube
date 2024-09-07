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

#include "core/pch.h"

#include "glm/glm.hpp"

#include "client/computeShader.h"

namespace client {

struct BloomMip {
    glm::vec2 size;
    glm::ivec2 intSize;
    unsigned int texture;
};

class Bloom {
public:
private:
    ComputeShader& m_downsampleShader;
    ComputeShader& m_upsampleShader;
    ComputeShader& m_blitShader;
    std::vector<BloomMip> m_mipChain;
    unsigned int m_srcTexture;

    void renderDownsamples();
    void renderUpsamples(float filterRadius);
public:
    Bloom(unsigned int srcTexture, unsigned int windowSize[2], unsigned int mipChainLength,
        ComputeShader& downsampleShader, ComputeShader& upsampleShader, ComputeShader& blitShader);
    ~Bloom();
    inline const std::vector<BloomMip>& getMipChain() const {
        return m_mipChain;
    }
    void render(float filterRadius, float strength);
};

}  // namespace client