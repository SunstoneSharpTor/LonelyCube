/*
  Lonely Cube, a voxel game
  Copyright (C) g 2024-2025 Bertie Cartwright

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

#include "client/graphics/renderer.h"

namespace lonelycube::client {

class Texture {
private:
    uint32_t m_rendererID;
    std::string m_filePath;
    uint8_t* m_localBuffer;
    int m_width, m_height, m_BPP;
public:
    Texture(const std::string& path);
    ~Texture();

    void bind(uint32_t slot = 0) const;
    void unbind() const;

    inline int getWidth() const { return m_width; }
    inline int getHeight() const { return m_height; }
};

}  // namespace lonelycube::client
