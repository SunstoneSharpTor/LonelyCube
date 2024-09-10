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

namespace constants
{
    constexpr unsigned short CHUNK_SIZE{ 32 };

    constexpr unsigned int WORLD_BORDER_DISTANCE{ 128000 };

    constexpr unsigned int BORDER_DISTANCE_U_B{ (WORLD_BORDER_DISTANCE / CHUNK_SIZE + 1) * CHUNK_SIZE }; //upper bound for world border distance

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

    constexpr float CUBE_FACE_POSITIONS[72] = { 1.0004f, -0.0004f, 0.0f,
                                               -0.0004f, -0.0004f, 0.0f,
                                               -0.0004f,  1.0004f, 0.0f,
                                                1.0004f,  1.0004f, 0.0f,
                                                
                                               -0.0004f, -0.0004f, 1.0f,
                                                1.0004f, -0.0004f, 1.0f,
                                                1.0004f,  1.0004f, 1.0f,
                                               -0.0004f,  1.0004f, 1.0f,
                                                
                                                0.0f, -0.0004f, -0.0004f,
                                                0.0f, -0.0004f,  1.0004f,
                                                0.0f,  1.0004f,  1.0004f,
                                                0.0f,  1.0004f, -0.0004f,
                                                
                                                1.0f, -0.0004f,  1.0004f,
                                                1.0f, -0.0004f, -0.0004f,
                                                1.0f,  1.0004f, -0.0004f,
                                                1.0f,  1.0004f,  1.0004f,
                                                
                                               -0.0004f, 0.0f, -0.0004f,
                                                1.0004f, 0.0f, -0.0004f,
                                                1.0004f, 0.0f,  1.0004f,
                                               -0.0004f, 0.0f,  1.0004f,
                                                
                                               -0.0004f, 1.0f,  1.0004f,
                                                1.0004f, 1.0f,  1.0004f,
                                                1.0004f, 1.0f, -0.0004f,
                                               -0.0004f, 1.0f, -0.0004f };

    constexpr float X_FACE_POSITIONS[48] = { 1.0f, 0.0f, 1.0f,
                                             0.0f, 0.0f, 0.0f,
                                             0.0f, 1.0f, 0.0f,
                                             1.0f, 1.0f, 1.0f,
                                             
                                             0.0f, 0.0f, 0.0f,
                                             1.0f, 0.0f, 1.0f,
                                             1.0f, 1.0f, 1.0f,
                                             0.0f, 1.0f, 0.0f,
                                             
                                             1.0f, 0.0f, 0.0f,
                                             0.0f, 0.0f, 1.0f,
                                             0.0f, 1.0f, 1.0f,
                                             1.0f, 1.0f, 0.0f,
                                             
                                             0.0f, 0.0f, 1.0f,
                                             1.0f, 0.0f, 0.0f,
                                             1.0f, 1.0f, 0.0f,
                                             0.0f, 1.0f, 1.0f };

    constexpr float WIREFRAME_CUBE_FACE_POSITIONS[24] = { -0.001f, -0.001f, -0.001f,
                                                           1.001f, -0.001f, -0.001f,
                                                           1.001f, -0.001f,  1.001f,
                                                          -0.001f, -0.001f,  1.001f,
                                                          
                                                          -0.001f,  1.001f,  1.001f,
                                                           1.001f,  1.001f,  1.001f,
                                                           1.001f,  1.001f, -0.001f,
                                                          -0.001f,  1.001f, -0.001f };

    constexpr unsigned int CUBE_WIREFRAME_IB[16] = { 0, 1, 2, 3, 0,
                                                     7, 6, 1, 6, 5, 2, 5, 4, 3, 4, 7 };

    constexpr bool collideable[8] = { false, //air
                                      true, //dirt
                                      true, //grass
                                      true, //stone
                                      false, //water
                                      true, //oak log
                                      true, //oak leaves
                                      false //tall grass
    };

    constexpr bool transparent[8] = { true, //air
                                      false, //dirt
                                      false, //grass
                                      false, //stone
                                      true, //water
                                      false, //oak log
                                      true, //oak leaves
                                      true //tall grass
    };

    constexpr bool castsShadows[8] = { false, //air
                                       true, //dirt
                                       true, //grass
                                       true, //stone
                                       false, //water
                                       true, //oak log
                                       false, //oak leaves
                                       false //tall grass
    };

    constexpr bool dimsLight[8] = { false, //air
                                    false, //dirt
                                    false, //grass
                                    false, //stone
                                    true, //water
                                    false, //oak log
                                    true, //oak leaves
                                    false //tall grass
    };

    constexpr bool cubeMesh[8] = { true, //air
                               true, //dirt
                               true, //grass
                               true, //stone
                               true, //water
                               true, //oak log
                               true, //oak leaves
                               false //tall grass
    };

    constexpr bool xMesh[8] = { false, //air
                                false, //dirt
                                false, //grass
                                false, //stone
                                false, //water
                                false, //oak log
                                false, //oak leaves
                                true //tall grass
    };

    constexpr float shadowReceiveAmount[8] = { 0.45f, //air
                                               0.45f, //dirt
                                               0.45f, //grass
                                               0.45f, //stone
                                               0.45f, //water
                                               0.45f, //oak log
                                               0.7f, //oak leaves
                                               0.45f //tall grass
    };
}
#endif