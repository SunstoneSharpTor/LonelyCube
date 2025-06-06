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

#include "enet/enet.h"

#include "core/pch.h"

#include "client/clientWorld.h"
#include "core/packet.h"

namespace lonelycube::client {

class ClientNetworking {
private:
    ENetHost* m_host;
    ENetPeer* m_peer;
    std::mutex m_hostMtx;
public:
    bool establishConnection(const std::string& serverIP, uint16_t renderDistance);
    void disconnect(ClientWorld& mainWorld);
    void receivePacket(ENetPacket* packet, ClientWorld& mainWorld);
    bool receiveEvents(ClientWorld& mainWorld);
    ENetPeer* getPeer() {
        return m_peer;
    }
    ENetHost* getHost() {
      return m_host;
    }
    std::mutex& getMutex() {
        return m_hostMtx;
    }
};

}  // namespace lonelycube::client
