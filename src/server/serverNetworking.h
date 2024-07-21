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

#include "core/pch.h"

#include "enet/enet.h"

#include "core/serverWorld.h"

namespace server {

class ServerNetworking {
public:
    static bool initServer(ENetAddress& address, ENetHost*& server);
    static void receivePacket(ENetPacket* packet, ENetPeer* peer, ServerWorld& mainWorld);
    static void receiveEvents(ENetHost* server, ServerWorld& mainWorld);
};

}  // namespace server