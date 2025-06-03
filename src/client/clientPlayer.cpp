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

#include "client/input.h"
#include "core/pch.h"

#include "client/clientPlayer.h"
#include "core/constants.h"
#include "core/packet.h"

namespace lonelycube::client {

const float ClientPlayer::m_hitBoxCorners[36] = { 0.0f, 0.0f, 0.0f,
                                            0.6f, 0.0f, 0.0f,
                                            0.6f, 0.0f, 0.6f,
                                            0.0f, 0.0f, 0.6f,
                                            0.0f, 0.9f, 0.0f,
                                            0.6f, 0.9f, 0.0f,
                                            0.6f, 0.9f, 0.6f,
                                            0.0f, 0.9f, 0.6f,
                                            0.0f, 1.8f, 0.0f,
                                            0.6f, 1.8f, 0.0f,
                                            0.6f, 1.8f, 0.6f,
                                            0.0f, 1.8f, 0.6f };

const int ClientPlayer::m_directions[18] = { 1, 0, 0,
                                      -1, 0, 0,
                                       0, 1, 0,
                                       0,-1, 0,
                                       0, 0, 1,
                                       0, 0,-1 };

ClientPlayer::ClientPlayer(
    const IVec3& playerPos, ClientWorld* mainWorld, ResourcePack& resourcePack
) : m_mainWorld(mainWorld), m_resourcePack(resourcePack)
{
    m_lastMousePos[0] = m_lastMousePos[1] = 0;
    m_playing = false;
    m_lastPlaying = false;
    m_paused = true;
    m_cursorNeedsReleasing = false;

    viewCamera = Camera(glm::vec3(0.5f, 0.5f, 0.5f));

    m_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    for (uint8_t i = 0; i < 3; i++) {
        m_hitboxMinBlock[i] = playerPos[i];
    }
    m_hitboxMinOffset = glm::vec3(0.5f, 0.5f, 0.5f);

    for (uint8_t i = 0; i < 3; i++) {
        cameraBlockPosition[i] = m_hitboxMinBlock[i];
        viewCamera.position[i] = m_hitboxMinOffset[i] + 0.3f;
    }

    viewCamera.position[1] += 1.32f;

    m_touchGround = false;

    m_yaw = 90.0f;
    m_pitch = 0.0f;
    viewCamera.updateRotationVectors(m_yaw, m_pitch);

    m_timeSinceBlockPlace = 0.0f;
    m_timeSinceBlockBreak = 0.0f;
    m_timeSinceLastJump = 0.0f;
    m_timeSinceTouchGround = 1000.0f;
    m_timeSinceTouchWater = 1000.0f;
    m_timeSinceLastSpace = 1000.0f;
    zoom = false;
    m_fly = false;
    m_lastSpace = false;
    m_crouch = false;
    m_touchWater = false;

    m_blockHolding = 1;

    m_time = m_physicsTime = 0.0;
}

void ClientPlayer::processUserInput(
    GLFWwindow* window, int* windowDimensions, double dt, ClientNetworking& networking
) {
    // Handle window losing focus
    bool windowFocus = glfwGetWindowAttrib(window, GLFW_FOCUSED);
    if (!windowFocus && m_playing)
    {
        m_playing = m_lastPlaying = false;
        m_cursorLeftWindow = false;
        m_cursorNeedsReleasing = true;
    }
    bool cursorInWindow = glfwGetWindowAttrib(window, GLFW_HOVERED);
    if (m_cursorNeedsReleasing)
    {
        if (!m_cursorLeftWindow && !cursorInWindow)
            m_cursorLeftWindow = true;

        if (windowFocus || cursorInWindow && m_cursorLeftWindow)
        {
            LOG("Releasing cursor");
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            m_cursorNeedsReleasing = false;
            cursorInWindow = glfwGetWindowAttrib(window, GLFW_HOVERED);
        }
    }

    // Refocus the window if it has just been clicked
    if (!m_playing)
    {
        if (input::anyMouseButtonPressed())
        {
            focus(window, windowDimensions);
        }
    }
    if (!m_paused)
    {
        m_timeSinceBlockBreak += dt;
        m_timeSinceBlockPlace += dt;
        m_timeSinceLastJump += dt;
        m_timeSinceLastSpace += dt;
    }

    double localCursorPosition[2];
    glfwGetCursorPos(window, &localCursorPosition[0], &localCursorPosition[1]);
    glm::ivec2 windowSize;
    glfwGetWindowSize(window, &windowSize.x, &windowSize.y);

    if (m_playing)  // Update camera view
    {
        if (m_lastPlaying && windowSize == m_lastWindowSize)
        {
            m_yaw += (localCursorPosition[0] - m_lastMousePos[0]) * 0.05f;
            m_pitch -= (localCursorPosition[1] - m_lastMousePos[1]) * 0.05f;
            if (m_pitch <= -90.0f)
                m_pitch = -89.999f;
            else if (m_pitch >= 90.0f)
                m_pitch = 89.999f;

            viewCamera.updateRotationVectors(m_yaw, m_pitch);
        }
        m_lastMousePos[0] = localCursorPosition[0];
        m_lastMousePos[1] = localCursorPosition[1];
        m_lastWindowSize = windowSize;
    }
    m_mainWorld->updateViewCamera(viewCamera);

    //  Break / place blocks
    if (m_lastPlaying)
    {
        m_pauseMouseState &= 2 | (1 * (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)));
        m_pauseMouseState &= 1 | (2 * (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)));
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) && !(m_pauseMouseState & 1))
        {
            if (m_timeSinceBlockBreak >= 0.2f)
            {
                int breakBlockCoords[3];
                int placeBlockCoords[3];
                if (m_mainWorld->shootRay(
                    viewCamera.position, cameraBlockPosition, viewCamera.front, breakBlockCoords,
                    placeBlockCoords
                )) {
                    m_timeSinceBlockBreak = 0.0f;
                    uint8_t itemType = m_mainWorld->integratedServer.chunkManager.getBlock(breakBlockCoords);
                    m_mainWorld->replaceBlock(breakBlockCoords, 0);
                    m_mainWorld->integratedServer.getEntityManager().addItem(
                        itemType, breakBlockCoords, Vec3(0.5f, 0.5f, 0.5f)
                    );
                    if (!m_mainWorld->isSinglePlayer())
                    {
                        Packet<int, 4> payload(m_mainWorld->getClientID(), PacketType::BlockReplaced, 4);
                        for (int i = 0; i < 3; i++)
                            payload[i] = breakBlockCoords[i];
                        payload[3] = 0;
                        networking.getMutex().lock();
                        ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(networking.getPeer(), 0, packet);
                        networking.getMutex().unlock();
                    }
                }
            }
        }
        else
        {
            m_timeSinceBlockBreak = 0.2f;
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) && !(m_pauseMouseState & 2))
        {
            if (m_timeSinceBlockPlace >= 0.2f)
            {
                int breakBlockCoords[3];
                int placeBlockCoords[3];
                if (m_mainWorld->shootRay(
                    viewCamera.position, cameraBlockPosition, viewCamera.front, breakBlockCoords,
                    placeBlockCoords) != 0)
                {
                    if ((!intersectingBlock(placeBlockCoords)) || (!m_resourcePack.getBlockData(m_blockHolding).collidable))
                    {
                        //TODO:
                        //(investigate) fix the replace block function to scan the unmeshed chunks too
                        m_timeSinceBlockPlace = 0.0f;
                        m_mainWorld->replaceBlock(placeBlockCoords, m_blockHolding);
                        if (!m_mainWorld->isSinglePlayer())
                        {
                            Packet<int, 4> payload(m_mainWorld->getClientID(), PacketType::BlockReplaced, 4);
                            for (int i = 0; i < 3; i++)
                                payload[i] = placeBlockCoords[i];
                            payload[3] = m_blockHolding;
                            networking.getMutex().lock();
                            ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_RELIABLE);
                            enet_peer_send(networking.getPeer(), 0, packet);
                            networking.getMutex().unlock();
                        }
                    }
                }
            }
        }
        else
        {
            m_timeSinceBlockPlace = 0.2f;
        }
    }

    // Movement
    glm::vec3 force(0);
    float movementSpeed;
    float swimSpeed;
    float sprintSpeed;
    bool sprint = false;
    m_crouch = false;
    if (!m_paused)
    {
        if (m_touchGround && m_fly)
            m_fly = false;

        m_timeSinceTouchGround = (m_timeSinceTouchGround + dt) * (!m_touchGround);
        m_timeSinceTouchWater = (m_timeSinceTouchWater + dt) * (!m_touchWater);
        if (m_fly)
        {
            movementSpeed = 100.0f;
            swimSpeed = 100.0f;
            sprintSpeed = 100.0f;
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
            {
                sprintSpeed = 1200.0f;
                sprint = true;
            }
        }
        else
        {
            force[1] -= 28.0;
            movementSpeed = 42.5f;
            swimSpeed = 70.0f;
            sprintSpeed = 42.5f;
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
            {
                sprintSpeed = 58.0f;
                sprint = true;
            }
            movementSpeed = std::max(abs(m_velocity[1] * 1.5f), movementSpeed - std::min(m_timeSinceTouchGround, m_timeSinceTouchWater) * 16);
            sprintSpeed = std::max(std::abs(m_velocity[1] * 1.5f), sprintSpeed - std::min(m_timeSinceTouchGround, m_timeSinceTouchWater) * 16);
        }
    }
    if (m_lastPlaying)
    {
        //keyboard input
        if (glfwGetKey(window, GLFW_KEY_W))
        {
            if (glfwGetKey(window, GLFW_KEY_A) != glfwGetKey(window, GLFW_KEY_D))
            {
                float fac;
                if (sprint)
                    fac = sprintSpeed / std::sqrt(sprintSpeed * sprintSpeed + movementSpeed * movementSpeed);
                else
                    fac = 0.707107f;
                sprintSpeed *= fac;
                movementSpeed *= fac;
            }
            force -= sprintSpeed * glm::normalize(glm::cross(viewCamera.right, viewCamera.worldUp));
        }
        if (glfwGetKey(window, GLFW_KEY_S)) {
            if (glfwGetKey(window, GLFW_KEY_A) != glfwGetKey(window, GLFW_KEY_D))
                movementSpeed *= 0.707107f;
            force += movementSpeed * glm::normalize(glm::cross(viewCamera.right, viewCamera.worldUp));
        } if (glfwGetKey(window, GLFW_KEY_A)) {
            force -= movementSpeed * viewCamera.right;
        } if (glfwGetKey(window, GLFW_KEY_D)) {
            force += movementSpeed * viewCamera.right;
        } if (glfwGetKey(window, GLFW_KEY_SPACE)) {
            if ((m_timeSinceLastSpace < 0.4f) && !m_lastSpace) {
                m_fly = !m_fly;
                m_velocity[1] = 0.0f;
                force[1] = 0.0f;
                m_timeSinceLastSpace = 1000.0f;
            }
            else if (!m_lastSpace) {
                m_timeSinceLastSpace = 0.0f;
            }
            m_lastSpace = true;

            if (!m_fly) {
                if (m_touchWater)
                    force[1] += swimSpeed;
                else if (m_touchGround) {
                    m_velocity[1] = 8.0f * viewCamera.worldUp[1];
                    force[1] = 0.0f;
                    m_timeSinceLastJump = 0.0f;
                }
            }
            else {
                force += sprintSpeed * viewCamera.worldUp;
            }
        } else {
            m_lastSpace = false;
        } if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
            if (m_fly)
                force -= sprintSpeed * viewCamera.worldUp;
            else
                m_crouch = true;
        } if (glfwGetKey(window, GLFW_KEY_1)) {
            m_blockHolding = 1;
        } if (glfwGetKey(window, GLFW_KEY_2)) {
            m_blockHolding = 2;
        } if (glfwGetKey(window, GLFW_KEY_3)) {
            m_blockHolding = 3;
        } if (glfwGetKey(window, GLFW_KEY_4)) {
            m_blockHolding = 4;
        } if (glfwGetKey(window, GLFW_KEY_5)) {
            m_blockHolding = 5;
        } if (glfwGetKey(window, GLFW_KEY_6)) {
            m_blockHolding = 6;
        } if (glfwGetKey(window, GLFW_KEY_7)) {
            m_blockHolding = 7;
        } if (glfwGetKey(window, GLFW_KEY_8)) {
            m_blockHolding = 8;
        } if (glfwGetKey(window, GLFW_KEY_9)) {
            m_blockHolding = 9;
        } if (glfwGetKey(window, GLFW_KEY_C)) {
            zoom = true;
        } else {
            zoom = false;
        }
    }
    if (!m_paused)
    {
        m_physicsTime += dt;
        while (m_time < m_physicsTime - constants::visualTickTime)
        {
            //calculate friction
            glm::vec3 friction = (m_velocity) * -10.0f * (m_touchWater * (!m_fly) * 0.8f + 1.0f);
            friction[1] *= m_fly || m_touchWater;
            m_velocity += (force + friction) * constants::visualTickTime;

            resolveHitboxCollisions(constants::visualTickTime);

            //update camera position
            for (uint8_t i = 0; i < 3; i++)
            {
                cameraBlockPosition[i] = m_hitboxMinBlock[i];
                viewCamera.position[i] = m_hitboxMinOffset[i] + 0.3f;
            }

            viewCamera.position[1] += 1.32f;

            int roundedPos;
            for (uint8_t i = 0; i < 3; i++)
            {
                roundedPos = viewCamera.position[i];
                cameraBlockPosition[i] += roundedPos;
                viewCamera.position[i] -= roundedPos;
            }
            m_time += constants::visualTickTime;
        }
    }

    m_lastPlaying = m_playing;
}

void ClientPlayer::unfocus(GLFWwindow* window, int* windowDimensions)
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPos(window, (double)windowDimensions[0] / 2, (double)windowDimensions[1] / 2);
    m_playing = m_lastPlaying = false;
    m_paused = true;
}

void ClientPlayer::focus(GLFWwindow* window, int* windowDimensions)
{
    glfwSetCursorPos(window, (double)windowDimensions[0] / 2, (double)windowDimensions[1] / 2);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    m_pauseMouseState = 0;
    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT))
        m_pauseMouseState = 1;
    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT))
        m_pauseMouseState |= 2;

    m_playing = true;
    m_paused = false;
}

void ClientPlayer::resolveHitboxCollisions(float DT) {
    m_touchGround = false;
    bool lastTouchWater = m_touchWater;
    m_touchWater = false;
    float penetration;
    float minPenetration;
    uint8_t axisOfLeastPenetration = 2;
    int position[3];
    int neighbouringBlockPosition[3];

    //TODO: investigate the effect of changing subdivisions (causes bugs)
    float subdivisions = 32;

    for (uint8_t subdivision = 0; subdivision < subdivisions; subdivision++) {
        m_hitboxMinOffset += m_velocity * DT / subdivisions;

        for (uint8_t i = 0; i < 3; i++) {
            m_hitboxMinBlock[i] += floor(m_hitboxMinOffset[i]);
            m_hitboxMinOffset[i] -= floor(m_hitboxMinOffset[i]);
        }

        bool resolved = false;
        while (!resolved) {
            axisOfLeastPenetration = 2;
            resolved = true;
            minPenetration = 1000.0f;
            for (uint8_t hitboxCorner = 0; hitboxCorner < 12; hitboxCorner++) {
                for (uint8_t i = 0; i < 3; i++) {
                    position[i] = m_hitboxMinBlock[i] + floor(m_hitboxMinOffset[i] + m_hitBoxCorners[hitboxCorner * 3 + i]);
                }

                int16_t blockType = m_mainWorld->integratedServer.chunkManager.getBlock(position);
                if (m_resourcePack.getBlockData(blockType).collidable) {
                    for (uint8_t direction = 0; direction < 6; direction++) {
                        penetration = m_hitboxMinOffset[direction / 2] + m_hitBoxCorners[hitboxCorner * 3 + direction / 2] - floor(m_hitboxMinOffset[direction / 2] + m_hitBoxCorners[hitboxCorner * 3 + direction / 2]);
                        if (direction % 2 == 0) {
                            penetration = 1.0f - penetration;
                        }
                        if (penetration < minPenetration) {
                            for (uint8_t i = 0; i < 3; i++) {
                                neighbouringBlockPosition[i] = position[i] + m_directions[direction * 3 + i];
                            }
                            blockType = m_mainWorld->integratedServer.chunkManager.getBlock(neighbouringBlockPosition);
                            if ((!m_resourcePack.getBlockData(blockType).collidable) && (m_velocity[direction / 2] != 0.0f)) {
                                minPenetration = penetration;
                                axisOfLeastPenetration = direction;
                                resolved = false;
                            }
                        }
                    }
                }
                else {
                    if (blockType == 4) {
                        if (lastTouchWater) {
                            m_touchWater = true;
                        }
                        else {
                            if (hitboxCorner > 3) {
                                m_touchWater = true;
                            }
                        }
                    }
                }
            }

            if (!resolved) {
                m_hitboxMinOffset -= m_velocity * DT / subdivisions;
                m_velocity[axisOfLeastPenetration / 2] = 0.0f;
                m_hitboxMinOffset += m_velocity * DT / subdivisions;
                if (axisOfLeastPenetration == 2) {
                    m_touchGround = true;
                }
            }
        }
    }
}

bool ClientPlayer::collidingWithBlock() {
    int position[3];
    for (uint8_t hitboxCorner = 0; hitboxCorner < 12; hitboxCorner++) {
        for (uint8_t i = 0; i < 3; i++) {
            position[i] = m_hitboxMinBlock[i] + floor(m_hitboxMinOffset[i] + m_hitBoxCorners[hitboxCorner * 3 + i]);
        }

        int16_t blockType = m_mainWorld->integratedServer.chunkManager.getBlock(position);
        if (m_resourcePack.getBlockData(blockType).collidable) {
            return true;
        }
    }
    return false;
}

bool ClientPlayer::intersectingBlock(int* blockPos) {
    bool intersecting;
    int position;
    for (uint8_t hitboxCorner = 0; hitboxCorner < 12; hitboxCorner++) {
        intersecting = true;
        for (uint8_t i = 0; i < 3; i++) {
             position = m_hitboxMinBlock[i] + floor(m_hitboxMinOffset[i] + m_hitBoxCorners[hitboxCorner * 3 + i]);
             if (position != blockPos[i]) {
                 intersecting = false;
             }
        }
        if (intersecting) {
            return true;
        }
    }
    return false;
}

}  // namespace lonelycube::client
