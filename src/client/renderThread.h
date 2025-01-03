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

#include "client/clientNetworking.h"
#include "client/clientPlayer.h"
#include "client/clientWorld.h"

namespace client {

class RenderThread {
private:
    ClientWorld* m_mainWorld;
    bool* m_chunkLoaderThreadsRunning;
    ClientPlayer* m_mainPlayer;
    ClientNetworking& m_networking;
    int* m_frameTime;
    GLFWwindow* m_window;

    float calculateBrightness(const float* points, uint32_t numPoints, uint32_t time);

    // void setSDLIcon(SDL_Window* window);
public:
    RenderThread(ClientWorld* mainWorld, bool* chunkLoaderThreadsRunning, ClientPlayer*
    mainPlayer, ClientNetworking& networking, int* frameTime) : m_mainWorld(mainWorld),
    m_chunkLoaderThreadsRunning(chunkLoaderThreadsRunning), m_mainPlayer(mainPlayer),
    m_networking(networking), m_frameTime(frameTime) {}
    void go(bool* running);
};

}  // namespace client
