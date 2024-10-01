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

#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace constants {

constexpr unsigned short CHUNK_SIZE{ 32 };

constexpr unsigned int WORLD_BORDER_DISTANCE{ 128000 };

//upper bound for world border distance as a multiple of CHUNK_SIZE
constexpr unsigned int BORDER_DISTANCE_U_B{ (WORLD_BORDER_DISTANCE / CHUNK_SIZE + 1) * CHUNK_SIZE };

constexpr unsigned char skyLightMaxValue{ 15 };

constexpr int visualTPS{ 240 };

constexpr unsigned int DAY_LENGTH{ 12000 };

constexpr unsigned int NUM_GROUND_LUMINANCE_POINTS{ 13 };
constexpr float GROUND_LUMINANCE[NUM_GROUND_LUMINANCE_POINTS * 2] = {
// Time | Luminance
    1800.0f, 0.000025f,
    2400.0f, 0.0001f,
    2500.0f, 0.0003f,
    2850.0f, 0.0025f,
    3000.0f, 0.04f,
    5000.0f, 10.0f,
    6000.0f, 11.0f,
    7000.0f, 10.0f,
    9000.0f, 0.04f,
    9150.0f, 0.0025f,
    9500.0f, 0.0003f,
    9600.0f, 0.0001f,
    10200.0f, 0.000025f };

constexpr float WIREFRAME_CUBE_FACE_POSITIONS[24] = { -0.001f, -0.001f, -0.001f,
                                                        1.001f, -0.001f, -0.001f,
                                                        1.001f, -0.001f,  1.001f,
                                                        -0.001f, -0.001f,  1.001f,
                                                        
                                                        -0.001f,  1.001f,  1.001f,
                                                        1.001f,  1.001f,  1.001f,
                                                        1.001f,  1.001f, -0.001f,
                                                        -0.001f,  1.001f, -0.001f };

constexpr unsigned int CUBE_WIREFRAME_IB[16] = { 0, 1, 2, 3, 0, 7, 6, 1, 6, 5, 2, 5, 4, 3, 4, 7 };

}
#endif