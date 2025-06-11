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

#include "client/clientNetworking.h"
#include "client/clientPlayer.h"
#include "client/clientWorld.h"
#include "client/graphics/renderer.h"

namespace lonelycube::client {

class Game
{
public:
    Game(
        Renderer& renderer, bool multiplayer, const std::string& serverIP, int renderDistance,
        uint64_t seed
    );
    ~Game();
    void processInput(double dt);
    void focus();
    void unfocus();
    void renderFrame(double dt);
    void queueDebugText(int guiScale, int FPS);

    inline ClientWorld& getWorld()
    {
        return m_mainWorld;
    }
    inline ClientPlayer& getPlayer()
    {
        return m_mainPlayer;
    }

private:
    bool m_multiplayer;
    bool m_running;
    Renderer& m_renderer;
    ClientNetworking m_networking;
    ClientWorld m_mainWorld;
    ClientPlayer m_mainPlayer;
    std::vector<bool> m_chunkLoaderThreadsRunning;
    std::thread m_logicWorker;

    float m_exposure = 0.0;
    float m_toneMapTimeByDTs = 0.0;
    bool m_windowLastFocus = false;
};

}  // namespace lonelycube::client
