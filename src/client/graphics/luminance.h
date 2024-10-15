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

#pragma once

#include "core/pch.h"

#include "glm/glm.hpp"

#include "client/graphics/computeShader.h"

namespace client {

struct LuminanceMip {
    glm::vec2 size;
    glm::ivec2 intSize;
    unsigned int texture;
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
    Luminance(unsigned int srcTexture, unsigned int windowSize[2], ComputeShader& luminanceShader,
        ComputeShader& downsampleShader);
    ~Luminance();
    float calculate();
    void resize(unsigned int windowSize[2]);
};

}  // namespace client

#endif