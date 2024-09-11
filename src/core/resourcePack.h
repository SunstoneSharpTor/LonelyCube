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

struct BlockModel {

};

struct BlockData {
    std::string name;
    unsigned char modelID;
    unsigned short faceTextures[6];
    bool transparent;
    bool dimsLight;
};

class ResourcePack {
private:
    BlockData m_blockData[256];
public:
    ResourcePack(std::filesystem::path resourcePackPath);
    const BlockData* const getBlockData() const {
        return m_blockData;
    }
};