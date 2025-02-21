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

#include "pch.h"

namespace lonelycube {

const uint8_t maxNumFaces = 6;

struct Face {
    int8_t lightingBlock;
    int8_t cullFace;
    bool ambientOcclusion;
    float UVcoords[4];
    float coords[12];
};

struct Model {
    uint8_t numFaces;
    std::string name;
    Face faces[maxNumFaces];
    float boundingBoxVertices[24];
};

struct BlockData {
    std::string name;
    Model* model;
    uint16_t faceTextureIndices[maxNumFaces];
    uint8_t blockLight;
    bool transparent;
    bool dimsLight;
    bool castsAmbientOcclusion;
    bool collidable;
};

class ResourcePack {
private:
    Model m_blockModels[256];
    BlockData m_blockData[256];

    bool isTrue(std::basic_istream<char>& stream) const;
public:
    static void getTextureCoordinates(float* coords, const float* textureBox, const int16_t
        textureNum);

    ResourcePack(std::filesystem::path resourcePackPath);
    const BlockData& getBlockData(uint8_t blockType) const {
        return m_blockData[blockType];
    }
};

}  // namespace lonelycube
