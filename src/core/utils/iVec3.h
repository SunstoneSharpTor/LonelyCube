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

struct IVec3 {
    int x, y, z;

    IVec3(int x, int y, int z) : x(x), y(y), z(z) {};
    IVec3(const int* position) : x(position[0]), y(position[1]), z(position[2]) {};
    IVec3() {};

    inline IVec3 operator+(const IVec3& other) const {
        return { x + other.x, y + other.y, z + other.z };
    }

    inline IVec3 operator-(const IVec3& other) const {
        return { x - other.x, y - other.y, z - other.z };
    }

    inline IVec3 operator*(const int scalar) const {
        return { x * scalar, y * scalar, z * scalar };
    }

    inline bool operator==(const IVec3& other) const {
        return (x == other.x) && (y == other.y) && (z == other.z);
    }

    inline void operator+=(const IVec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
    }

    inline void operator-=(const IVec3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
    }
};

namespace std {
    template<>
    struct hash<IVec3> {
        size_t operator()(const IVec3& key) const {
            return key.x * 8410720864772165619u
                ^ key.y * 8220336697060211182u
                ^ key.z * 11615669650507345147u;
        }
    };
}