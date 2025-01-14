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

#include "client/clientNetworking.h"
#include "client/clientWorld.h"
#include "client/graphics/camera.h"
#include "core/resourcePack.h"

#include <GLFW/glfw3.h>

namespace lonelycube::client {

class ClientPlayer {
private:
    static const float m_hitBoxCorners[36];
    static const int m_directions[18];

    double m_time;

    double m_lastMousePos[2];
    bool m_playing;
    bool m_lastPlaying;

    float m_timeSinceBlockPlace;
    float m_timeSinceBlockBreak;
    float m_timeSinceLastJump;
    float m_timeSinceLastSpace;
    float m_timeSinceTouchGround;
    float m_timeSinceTouchWater;

    bool m_fly;
    bool m_crouch;
    bool m_lastSpace;

    float m_yaw;
    float m_pitch;

    bool m_touchGround;
    bool m_touchWater;

    glm::vec3 m_velocity;

    int m_hitboxMinBlock[3];
    glm::vec3 m_hitboxMinOffset;
    ResourcePack& m_resourcePack;
    ClientWorld* m_mainWorld;

    void resolveHitboxCollisions(float DT);

    bool collidingWithBlock();

    bool intersectingBlock(int* blockPos);

public:
    Camera viewCamera;
    int cameraBlockPosition[3];
    bool zoom;

    uint16_t m_blockHolding;

    ClientPlayer(int* position, ClientWorld* newWorld, ResourcePack& resourcePack);

    void setWorldMouseData(GLFWwindow* window, uint32_t* windowDimensions);

    void processUserInput(GLFWwindow* window, uint32_t* windowDimensions, bool*
      windowLastFocus, bool* running, double currentTime, ClientNetworking& networking);
};

}  // namespace lonelycube::client
