/*
  Lonely Cube, a voxel game
  Copyright (C) 2024 Bertie Cartwright

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

#include "enet/enet.h"

#include "core/serverWorld.h"

namespace lonelycube::server {

class ServerNetworking {
private:
    ENetHost* m_host;
    std::mutex m_hostMtx;
public:
    bool initServer(ENetAddress& address);
    void receivePacket(ENetPacket* packet, ENetPeer* peer, ServerWorld<false>& mainWorld);
    void receiveEvents(ServerWorld<false>& mainWorld);
    ENetHost* getHost() {
        return m_host;
    }
    std::mutex& getMutex() {
        return m_hostMtx;
    }
};

}  // namespace lonelycube::server
