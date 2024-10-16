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

struct Position {
    int x, y, z;

    Position(int x, int y, int z) : x(x), y(y), z(z) {};
    Position(const int* position) : x(position[0]), y(position[1]), z(position[2]) {};
    Position() {};

    inline Position operator+(const Position& other) const {
        return { x + other.x, y + other.y, z + other.z };
    }

    inline Position operator-(const Position& other) const {
        return { x - other.x, y - other.y, z - other.z };
    }

    inline Position operator*(const int scalar) const {
        return { x * scalar, y * scalar, z * scalar };
    }

    inline bool operator==(const Position& other) const {
        return (x == other.x) && (y == other.y) && (z == other.z);
    }
};

namespace std {
    template<>
    struct hash<Position> {
        size_t operator()(const Position& key) const {
            return key.x * 8410720864772165619u
                ^ key.y * 8220336697060211182u
                ^ key.z * 11615669650507345147u;
        }
    };
}