/*
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

#include "client/clientPlayer.h"
#include "core/constants.h"
#include "core/packet.h"

#include "core/pch.h"

namespace client {

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

ClientPlayer::ClientPlayer(int* position, ClientWorld* newWorld) {
    m_keyboardState = SDL_GetKeyboardState(NULL);
    m_lastMousePos[0] = m_lastMousePos[1] = 0;
    m_playing = false;
    m_lastPlaying = false;
    m_pausedMouseState = 0u;

    m_mainWorld = newWorld;

    viewCamera = Camera(glm::vec3(0.5f, 0.5f, 0.5f));

    m_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    for (unsigned char i = 0; i < 3; i++) {
        m_hitboxMinBlock[i] = position[i];
    }
    m_hitboxMinOffset = glm::vec3(0.5f, 0.5f, 0.5f);

    for (unsigned char i = 0; i < 3; i++) {
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

    m_time = 0.0;
    m_lastMousePoll = 0.0f;
}



void ClientPlayer::processUserInput(SDL_Window* sdl_window, int* windowDimensions, bool* windowLastFocus, bool* running, double currentTime, ClientNetworking& networking) {
    float DT = 1.0f/(float)constants::visualTPS;
    float actualDT = floor((currentTime - m_time) / DT) * DT * (m_time != 0.0);
    if (m_playing) {
        m_timeSinceBlockBreak += actualDT;
        m_timeSinceBlockPlace += actualDT;
        m_timeSinceLastJump += actualDT;
        m_timeSinceLastSpace += actualDT;
    }

    Uint32 windowFlags = SDL_GetWindowFlags(sdl_window);
    int localCursorPosition[2];
    Uint32 mouseState = SDL_GetMouseState(&localCursorPosition[0], &localCursorPosition[1]);

    //break / place blocks
    if (m_lastPlaying) {
        m_pausedMouseState &= mouseState;
        mouseState &= ~m_pausedMouseState;
        if (mouseState & SDL_BUTTON(1)) {
            if (m_timeSinceBlockBreak >= 0.2f) {
                int breakBlockCoords[3];
                int placeBlockCoords[3];
                if (m_mainWorld->shootRay(viewCamera.position, cameraBlockPosition, viewCamera.front, breakBlockCoords, placeBlockCoords)) {
                    m_timeSinceBlockBreak = 0.0f;
                    m_mainWorld->replaceBlock(breakBlockCoords, 0);
                    Packet<int, 4> payload(m_mainWorld->getClientID(), PacketType::BlockReplaced, 4);
                    for (int i = 0; i < 3; i++) {
                        payload[i] = breakBlockCoords[i];
                    }
                    payload[3] = 0;
                    ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(networking.getPeer(), 0, packet);
                }
            }
        }
        else {
            m_timeSinceBlockBreak = 0.2f;
        }
        if (mouseState & SDL_BUTTON(3)) {
            if (m_timeSinceBlockPlace >= 0.2f) {
                int breakBlockCoords[3];
                int placeBlockCoords[3];
                if (m_mainWorld->shootRay(viewCamera.position, cameraBlockPosition, viewCamera.front, breakBlockCoords, placeBlockCoords) == 2) {
                    if ((!intersectingBlock(placeBlockCoords)) || (!constants::collideable[m_blockHolding])) {
                        //TODO:
                        //(investigate) fix the replace block function to scan the unmeshed chunks too
                        m_mainWorld->replaceBlock(placeBlockCoords, m_blockHolding);
                        Packet<int, 4> payload(m_mainWorld->getClientID(), PacketType::BlockReplaced, 4);
                        for (int i = 0; i < 3; i++) {
                            payload[i] = placeBlockCoords[i];
                        }
                        payload[3] = m_blockHolding;
                        ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(networking.getPeer(), 0, packet);
                        m_timeSinceBlockPlace = 0.0f;
                    }
                }
            }
        }
        else {
            m_timeSinceBlockPlace = 0.2f;
        }

        float movementSpeed;
        float swimSpeed;
        float sprintSpeed;
        glm::vec3 force = { 0.0f, 0.0f, 0.0f };
        bool sprint = false;
        m_crouch = false;

        if (m_touchGround && m_fly) {
            m_fly = false;
        }
        m_timeSinceTouchGround = (m_timeSinceTouchGround + actualDT) * (!m_touchGround);
        m_timeSinceTouchWater = (m_timeSinceTouchWater + actualDT) * (!m_touchWater);
        if (m_fly) {
            movementSpeed = 100.0f;
            swimSpeed = 100.0f;
            sprintSpeed = 100.0f;
            if (m_keyboardState[SDL_SCANCODE_LCTRL]) {
                sprintSpeed = 1200.0f;
                sprint = true;
            }
        }
        else {
            force[1] -= 28.0;
            movementSpeed = 42.5f;
            swimSpeed = 70.0f;
            sprintSpeed = 42.5f;
            if (m_keyboardState[SDL_SCANCODE_LCTRL]) {
                sprintSpeed = 58.0f;
                sprint = true;
            }
            movementSpeed = std::max(abs(m_velocity[1] * 1.5f), movementSpeed - std::min(m_timeSinceTouchGround, m_timeSinceTouchWater) * 16);
            sprintSpeed = std::max(std::abs(m_velocity[1] * 1.5f), sprintSpeed - std::min(m_timeSinceTouchGround, m_timeSinceTouchWater) * 16);
        }
        //keyboard input
        if (m_keyboardState[SDL_SCANCODE_W]) {
            if (m_keyboardState[SDL_SCANCODE_A] != m_keyboardState[SDL_SCANCODE_D]) {
                float fac;
                if (sprint) {
                    fac = sprintSpeed / std::sqrt(sprintSpeed * sprintSpeed + movementSpeed * movementSpeed);
                }
                else {
                    fac = 0.707107f;
                }
                sprintSpeed *= fac;
                movementSpeed *= fac;
            }
            force -= sprintSpeed * glm::normalize(glm::cross(viewCamera.right, viewCamera.worldUp));
        } if (m_keyboardState[SDL_SCANCODE_S]) {
            if (m_keyboardState[SDL_SCANCODE_A] != m_keyboardState[SDL_SCANCODE_D]) {
                movementSpeed *= 0.707107f;
            }
            force += movementSpeed * glm::normalize(glm::cross(viewCamera.right, viewCamera.worldUp));
        } if (m_keyboardState[SDL_SCANCODE_A]) {
            force -= movementSpeed * viewCamera.right;
        } if (m_keyboardState[SDL_SCANCODE_D]) {
            force += movementSpeed * viewCamera.right;
        } if (m_keyboardState[SDL_SCANCODE_SPACE]) {
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
                if (m_touchWater) {
                    force[1] += swimSpeed;
                }
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
        } if (m_keyboardState[SDL_SCANCODE_LSHIFT]) {
            if (m_fly) {
                force -= sprintSpeed * viewCamera.worldUp;
            }
            else {
                m_crouch = true;
            }
        } if (m_keyboardState[SDL_SCANCODE_1]) {
            m_blockHolding = 1;
        } if (m_keyboardState[SDL_SCANCODE_2]) {
            m_blockHolding = 2;
        } if (m_keyboardState[SDL_SCANCODE_3]) {
            m_blockHolding = 3;
        } if (m_keyboardState[SDL_SCANCODE_4]) {
            m_blockHolding = 4;
        } if (m_keyboardState[SDL_SCANCODE_5]) {
            m_blockHolding = 5;
        } if (m_keyboardState[SDL_SCANCODE_6]) {
            m_blockHolding = 6;
        } if (m_keyboardState[SDL_SCANCODE_7]) {
            m_blockHolding = 7;
        } if (m_keyboardState[SDL_SCANCODE_C]) {
            zoom = true;
        }
        else {
            zoom = false;
        } if (m_keyboardState[SDL_SCANCODE_ESCAPE]) {
            m_playing = false;
        }

        while (m_time < currentTime - DT) {

            //calculate friction
            glm::vec3 friction = (m_velocity) * -10.0f * (m_touchWater * (!m_fly) * 0.8f + 1.0f);
            friction[1] *= m_fly || m_touchWater;
            m_velocity += (force + friction) * DT;

            resolveHitboxCollisions(DT);

            //update camera position
            for (unsigned char i = 0; i < 3; i++) {
                cameraBlockPosition[i] = m_hitboxMinBlock[i];
                viewCamera.position[i] = m_hitboxMinOffset[i] + 0.3f;
            }

            viewCamera.position[1] += 1.32f;
            
            int roundedPos;
            for (unsigned char i = 0; i < 3; i++) {
                roundedPos = viewCamera.position[i];
                cameraBlockPosition[i] += roundedPos;
                viewCamera.position[i] -= roundedPos;
            }
            m_time += DT;
        }
    }
    while (m_time < currentTime - DT) {
        m_time += DT;
    }

    bool tempLastPlaying = m_lastPlaying;
    m_lastPlaying = m_playing;
    if (mouseState && (!m_playing)) {
        m_playing = true;
        m_pausedMouseState = mouseState;
    }

    if (!(windowFlags & SDL_WINDOW_INPUT_FOCUS)) {
        m_playing = false;
    }
    if (m_playing && !(tempLastPlaying)) {
        SDL_ShowCursor(SDL_DISABLE);
        SDL_SetWindowMouseGrab(sdl_window, SDL_TRUE);
    }
    else if ((!m_playing) && tempLastPlaying) {
        SDL_WarpMouseInWindow(sdl_window, windowDimensions[0] / 2, windowDimensions[1] / 2);
        SDL_ShowCursor(SDL_ENABLE);
        SDL_SetWindowMouseGrab(sdl_window, SDL_FALSE);
    }
}

void ClientPlayer::resolveHitboxCollisions(float DT) {
    m_touchGround = false;
    bool lastTouchWater = m_touchWater;
    m_touchWater = false;
    float penetration;
    float minPenetration;
    unsigned char axisOfLeastPenetration = 2;
    int position[3];
    int neighbouringBlockPosition[3];

    //TODO: investigate the effect of changing subdivisions (causes bugs)
    float subdivisions = 32;

    for (unsigned char subdivision = 0; subdivision < subdivisions; subdivision++) {
        m_hitboxMinOffset += m_velocity * DT / subdivisions;

        for (unsigned char i = 0; i < 3; i++) {
            m_hitboxMinBlock[i] += floor(m_hitboxMinOffset[i]);
            m_hitboxMinOffset[i] -= floor(m_hitboxMinOffset[i]);
        }

        bool resolved = false;
        while (!resolved) {
            axisOfLeastPenetration = 2;
            resolved = true;
            minPenetration = 1000.0f;
            for (unsigned char hitboxCorner = 0; hitboxCorner < 12; hitboxCorner++) {
                for (unsigned char i = 0; i < 3; i++) {
                    position[i] = m_hitboxMinBlock[i] + floor(m_hitboxMinOffset[i] + m_hitBoxCorners[hitboxCorner * 3 + i]);
                }

                short blockType = m_mainWorld->getBlock(position);
                if (constants::collideable[blockType]) {
                    for (unsigned char direction = 0; direction < 6; direction++) {
                        penetration = m_hitboxMinOffset[direction / 2] + m_hitBoxCorners[hitboxCorner * 3 + direction / 2] - floor(m_hitboxMinOffset[direction / 2] + m_hitBoxCorners[hitboxCorner * 3 + direction / 2]);
                        if (direction % 2 == 0) {
                            penetration = 1.0f - penetration;
                        }
                        if (penetration < minPenetration) {
                            for (unsigned char i = 0; i < 3; i++) {
                                neighbouringBlockPosition[i] = position[i] + m_directions[direction * 3 + i];
                            }
                            blockType = m_mainWorld->getBlock(neighbouringBlockPosition);
                            if ((!constants::collideable[blockType]) && (m_velocity[direction / 2] != 0.0f)) {
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
    for (unsigned char hitboxCorner = 0; hitboxCorner < 12; hitboxCorner++) {
        for (unsigned char i = 0; i < 3; i++) {
            position[i] = m_hitboxMinBlock[i] + floor(m_hitboxMinOffset[i] + m_hitBoxCorners[hitboxCorner * 3 + i]);
        }

        short blockType = m_mainWorld->getBlock(position);
        if (constants::collideable[blockType]) {
            return true;
        }
    }
    return false;
}

bool ClientPlayer::intersectingBlock(int* blockPos) {
    bool intersecting;
    int position;
    for (unsigned char hitboxCorner = 0; hitboxCorner < 12; hitboxCorner++) {
        intersecting = true;
        for (unsigned char i = 0; i < 3; i++) {
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

void ClientPlayer::setWorldMouseData(SDL_Window* window, int* windowDimensions) {
    m_mainWorld->setMouseData(&m_lastMousePoll,
                          &m_playing,
                          &m_lastPlaying,
                          &m_yaw,
                          &m_pitch,
                          m_lastMousePos,
                          &viewCamera,
                          window,
                          windowDimensions);
}

}  // namespace client