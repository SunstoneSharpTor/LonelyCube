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

#pragma once

#include "enet/enet.h"

#include "core/pch.h"

#include "client/clientWorld.h"
#include "core/packet.h"

namespace client {

class ClientNetworking {
private:
    ENetHost* m_host;
    ENetPeer* m_peer;
public:
    bool establishConnection(unsigned short renderDistance);
    void receivePacket(ENetPacket* packet, ClientWorld& mainWorld);
    void receiveEvents(ClientWorld& mainWorld);
    ENetPeer* getPeer() {
        return m_peer;
    }
    ENetHost* getHost() {
      return m_host;
    }
};

}  // namespace client