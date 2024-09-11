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

#include "core/resourcePack.h"

#include "core/pch.h"

ResourcePack::ResourcePack(std::filesystem::path resourcePackPath) {
    // Parse blocknames
    std::ifstream stream(resourcePackPath/"blocks/blockNames.json");
    stream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
    std::string name;
    unsigned int blockID = 0;
    while (!stream.eof()) {
        std::getline(stream, name, '"');
        m_blockData[blockID].name = name;
        blockID++;
        if (blockID > 255) {
            break;
        }
        stream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
    }
    stream.close();
}