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

namespace lonelycube::client {

class VertexBuffer {
private:
    uint32_t m_rendererID;
public:
    VertexBuffer();
    VertexBuffer(const void* data, uint32_t size);
    VertexBuffer(const void* data, uint32_t size, bool dynamic);
    ~VertexBuffer();

    void bind() const;
    void unbind() const;
    void update(const void* data, uint32_t size) const;
};

}  // namespace lonelycube::client
