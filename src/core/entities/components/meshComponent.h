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

// Class that stores a reference to a mesh in an instance of a mesh manager class, as well as a
// copy of the untranslated vertices
class MeshComponent {
public:
    uint32_t vertexBufferIndex;
    uint16_t numVertices;
    uint32_t indexBufferIndex;
    uint16_t numIndices;
    std::unique_ptr<float[]> untranslatedVertices;
};
