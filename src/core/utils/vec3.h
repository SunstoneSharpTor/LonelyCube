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

namespace lonelycube {

struct Vec3 {
private:
    float m_coords[3];

public:
    float& x = m_coords[0];
    float& y = m_coords[1];
    float& z = m_coords[2];

    Vec3(float x, float y, float z) : m_coords{ x, y, z } {}
    Vec3(const float* coords) : m_coords{ coords[0], coords[1], coords[2] } {}
    Vec3(const Vec3& other) : m_coords{ other.x, other.y, other.z } {}
    Vec3(const Vec3&& other) : m_coords{ other.x , other.y, other.z } {}
    Vec3() {}

    inline Vec3& operator=(const Vec3& other)
    {
        if (this == &other)
            return *this;
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    inline Vec3& operator=(const Vec3&& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    inline float& operator[](const int index)
    {
        return m_coords[index];
    }

    inline const float& operator[](const int index) const
    {
        return m_coords[index];
    }

    inline Vec3 operator+(const Vec3& other) const {
        return { x + other.x, y + other.y, z + other.z };
    }

    inline Vec3 operator-(const Vec3& other) const {
        return { x - other.x, y - other.y, z - other.z };
    }

    inline Vec3 operator*(const float scalar) const {
        return { x * scalar, y * scalar, z * scalar };
    }

    inline bool operator==(const Vec3& other) const {
        return (x == other.x) && (y == other.y) && (z == other.z);
    }

    inline void operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
    }

    inline void operator-=(const Vec3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
    }

    inline void operator*=(const float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
    }
};

}  // namespace lonelycube
