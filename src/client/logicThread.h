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

#include "core/pch.h"

#include "client/clientNetworking.h"
#include "client/clientPlayer.h"
#include "client/clientWorld.h"
#include "core/config.h"

namespace lonelycube::client {

class LogicThread
{
private:
    ClientWorld& m_mainWorld;
    bool* m_chunkLoaderThreadsRunning;
    ClientPlayer& m_mainPlayer;
    ClientNetworking& m_networking;
    Config& m_settings;
    bool& m_multiplayer;

public:
    LogicThread(ClientWorld& mainWorld, bool* chunkLoaderThreadsRunning, ClientPlayer&
        mainPlayer, ClientNetworking& networking, Config& settings, bool&
        multiplayer
    ) : m_mainWorld(mainWorld), m_chunkLoaderThreadsRunning(chunkLoaderThreadsRunning),
        m_mainPlayer(mainPlayer), m_networking(networking), m_settings(settings),
        m_multiplayer(multiplayer) {}

    void go(bool& running);
};

}  // namespace lonelycube::client
