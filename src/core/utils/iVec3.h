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

#include <cmath>
#include <ostream>
#include "core/utils/vec3.h"

namespace lonelycube {

struct IVec3 {
private:
    int m_coords[3];

public:
    int& x = m_coords[0];
    int& y = m_coords[1];
    int& z = m_coords[2];

    IVec3(int x, int y, int z) : m_coords{ x, y, z } {}
    IVec3(const int* coords) : m_coords{ coords[0], coords[1], coords[2] } {}
    IVec3(const Vec3 other) : m_coords{ static_cast<int>(std::floor(other.x)),
        static_cast<int>(std::floor(other.y)),
        static_cast<int>(std::floor(other.z)) } {}
    IVec3(const IVec3& other) : m_coords{ other.x, other.y, other.z } {}
    IVec3(const IVec3&& other) : m_coords{ other.x , other.y, other.z } {}
    IVec3() {}

    inline IVec3& operator=(const IVec3& other)
    {
        if (this == &other)
            return *this;
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    inline IVec3& operator=(const IVec3&& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    inline int& operator[](const int index)
    {
        return m_coords[index];
    }

    inline const int& operator[](const int index) const
    {
        return m_coords[index];
    }

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

std::ostream& operator<<(std::ostream& os, const lonelycube::IVec3& vec);

}  // namespace lonelycube

namespace std {
    template<>
    struct hash<lonelycube::IVec3> {
        size_t operator()(const lonelycube::IVec3& key) const {
            return key.x * 8410720864772165619u
                ^ key.y * 8220336697060211182u
                ^ key.z * 11615669650507345147u;
        }
    };
}
