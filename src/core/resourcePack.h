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

#include "pch.h"

const unsigned char maxNumFaces = 6;

struct Face {
    signed char lightingBlock;
    signed char cullFace;
    bool ambientOcclusion;
    float UVcoords[4];
    float coords[12];
};

struct BlockModel {
    unsigned char numFaces;
    std::string name;
    Face faces[maxNumFaces];
    float boundingBoxVertices[24];
};

struct BlockData {
    std::string name;
    BlockModel* model;
    unsigned short faceTextureIndices[maxNumFaces];
    bool transparent;
    bool dimsLight;
    bool collidable;
};

class ResourcePack {
private:
    BlockModel m_blockModels[256];
    BlockData m_blockData[256];

    bool isTrue(std::basic_istream<char>& stream) const;
public:
    ResourcePack(std::filesystem::path resourcePackPath);
    const BlockData& getBlockData(unsigned char blockType) const {
        return m_blockData[blockType];
    }
};