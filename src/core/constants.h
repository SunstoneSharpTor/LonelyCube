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

#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace lonelycube {

namespace constants {

constexpr uint16_t CHUNK_SIZE{ 32 };

constexpr uint32_t WORLD_BORDER_DISTANCE{ 128000 };

//upper bound for world border distance as a multiple of CHUNK_SIZE
constexpr uint32_t BORDER_DISTANCE_U_B{ (WORLD_BORDER_DISTANCE / CHUNK_SIZE + 1) * CHUNK_SIZE };

constexpr uint8_t skyLightMaxValue{ 15 };

constexpr uint8_t blockLightMaxValue{ 15 };

constexpr int visualTPS{ 240 };

constexpr uint32_t DAY_LENGTH{ 24000 };

constexpr int TICKS_PER_SECOND{ 20 };

constexpr uint32_t NUM_GROUND_LUMINANCE_POINTS{ 13 };
constexpr float GROUND_LUMINANCE[NUM_GROUND_LUMINANCE_POINTS * 2] = {
    // Time | Luminance
    3600.0f, 0.000025f,
    4800.0f, 0.0001f,
    5000.0f, 0.0003f,
    5700.0f, 0.0025f,
    6000.0f, 0.04f,
    10000.0f, 10.0f,
    12000.0f, 11.0f,
    14000.0f, 10.0f,
    18000.0f, 0.04f,
    18300.0f, 0.0025f,
    19000.0f, 0.0003f,
    19200.0f, 0.0001f,
    20400.0f, 0.000025f };

constexpr uint32_t CUBE_WIREFRAME_IB[16] = { 0, 1, 2, 3, 0, 7, 6, 1, 6, 5, 2, 5, 4, 3, 4, 7 };

}

#endif

}  // namespace lonelycube
