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

#include "client/graphics/computeShader.h"

namespace lonelycube::client {

struct LuminanceMip {
    glm::vec2 size;
    glm::ivec2 intSize;
    uint32_t texture;
};

class Luminance {
public:
private:
    ComputeShader& m_luminanceShader;
    ComputeShader& m_downsampleShader;
    std::vector<LuminanceMip> m_mipChain;
    LuminanceMip m_srcTexture;

    void createMips(glm::ivec2 srcTextureSize);
    void deleteMips();
public:
    Luminance(uint32_t srcTexture, uint32_t windowSize[2], ComputeShader& luminanceShader,
        ComputeShader& downsampleShader);
    ~Luminance();
    float calculate();
    void resize(uint32_t windowSize[2]);
};

}  // namespace lonelycube::client
