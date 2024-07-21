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

#include "clientNetworking.h"

#include "core/pch.h"
#include <cstring>

#include "client/clientWorld.h"

namespace client {

bool ClientNetworking::establishConnection(ENetHost*& client, ENetPeer*& peer, unsigned short renderDistance) {
    if (enet_initialize() != 0) {
        return EXIT_FAILURE;
    }
    
    client = enet_host_create(NULL, 1, 1, 0, 0);

    if (client == NULL) {
        return EXIT_FAILURE;
    }

    ENetAddress address;

    enet_address_set_host(&address, "127.0.0.1");
    address.port = 5555;

    peer = enet_host_connect(client, &address, 1, 0);
    if (peer == NULL) {
        return EXIT_FAILURE;
    }

    ENetEvent event;
    if ((enet_host_service(client, &event, 2000) > 0) && (event.type == ENET_EVENT_TYPE_CONNECT)) {
        std::cout << "Connection to 127.0.0.1 succeeded!" << std::endl;

        Packet<int, 1> payload(0, PacketType::ClientConnection, 1);
        payload[0] = renderDistance;
        ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, 0, packet);
        return true;
    }
    else {
        enet_peer_reset(peer);
        std::cout << "Connection to 127.0.0.1 failed." << std::endl;
        return false;
    }
}

void ClientNetworking::receivePacket(ENetPacket* packet, ENetPeer* peer, ClientWorld& mainWorld) {
    Packet<int, 0> head;
    memcpy(&head, packet->data, head.getSize());
    switch (head.getPacketType()) {
    case PacketType::ClientConnection:
    {
        Packet<int, 1> payload;
        memcpy(&payload, packet->data, packet->dataLength);
        mainWorld.setClientID(payload[0]);
        std::cout << "connected to server with clientID " << mainWorld.getClientID() << std::endl;
    }
    break;
    
    default:
        break;
    }
}

void ClientNetworking::receiveEvents(ENetHost* client, ClientWorld& mainWorld) {
    ENetEvent event;
    while(enet_host_service(client, &event, 0) > 0) {
        std::cout << "event\n";
        switch(event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                receivePacket(event.packet, event.peer, mainWorld);
                break;
        }
    }
}

}  // namespace client