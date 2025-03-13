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

#include "clientNetworking.h"

#include "core/log.h"
#include "core/packet.h"
#include <cstring>
#include <string>

#include "client/clientWorld.h"

namespace lonelycube::client {

bool ClientNetworking::establishConnection(std::string& serverIP, uint16_t renderDistance) {
    std::lock_guard<std::mutex> lock(m_hostMtx);
    if (enet_initialize() != 0) {
        return EXIT_FAILURE;
    }

    m_host = enet_host_create(NULL, 1, 2, 0, 0);

    if (m_host == NULL) {
        return EXIT_FAILURE;
    }

    ENetAddress address;

    enet_address_set_host(&address, serverIP.c_str());
    address.port = 5555;

    m_peer = enet_host_connect(m_host, &address, 2, 0);
    if (m_peer == NULL) {
        return EXIT_FAILURE;
    }

    ENetEvent event;
    if ((enet_host_service(m_host, &event, 2000) > 0) && (event.type == ENET_EVENT_TYPE_CONNECT)) {
        LOG("Connection to " + serverIP + " succeeded!");

        Packet<int, 1> payload(0, PacketType::ClientConnection, 1);
        payload[0] = renderDistance;
        ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(m_peer, 0, packet);
        return true;
    }
    else {
        enet_peer_reset(m_peer);
        LOG("Connection to 127.0.0.1 failed.");
        return false;
    }
}

void ClientNetworking::receivePacket(ENetPacket* packet, ClientWorld& mainWorld) {
    Packet<int, 0> head;
    memcpy(&head, packet->data, head.getSize());
    switch (head.getPacketType()) {
    case PacketType::ClientConnection:
    {
        Packet<uint16_t, 1> payload;
        memcpy(&payload, packet->data, packet->dataLength);
        mainWorld.setClientID(payload[0]);
        LOG("connected to server with clientID " + std::to_string(mainWorld.getClientID()));
    }
    break;
    case PacketType::ChunkSent:
    {
        Packet<uint8_t, 9 * constants::CHUNK_SIZE * constants::CHUNK_SIZE *
            constants::CHUNK_SIZE> payload;
        memcpy(&payload, packet->data, packet->dataLength);
        mainWorld.loadChunkFromPacket(payload);
    }
    break;
    case PacketType::BlockReplaced:
    {
        Packet<int, 4> payload;
        memcpy(&payload, packet->data, packet->dataLength);
        int blockCoords[3];
        memcpy(blockCoords, payload.getPayloadAddress(), 3 * sizeof(int));
        mainWorld.replaceBlock(blockCoords, payload[3]);
    }
    break;

    default:
        break;
    }
    enet_packet_destroy(packet);
}

void ClientNetworking::receiveEvents(ClientWorld& mainWorld) {
    ENetEvent event;
    m_hostMtx.lock();
    while(enet_host_service(m_host, &event, 0) > 0) {
        m_hostMtx.unlock();
        switch(event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                LOG("Received packet");
                receivePacket(event.packet, mainWorld);
                break;
            default:
                break;
        }
        m_hostMtx.lock();
    }
    m_hostMtx.unlock();
}

}  // namespace lonelycube::client
