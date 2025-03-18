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

#include "serverNetworking.h"

#include "core/pch.h"
#include <cstdint>
#include <cstring>
#include <thread>

#include "core/log.h"
#include "core/packet.h"
#include "core/serverWorld.h"

namespace lonelycube::server {

bool ServerNetworking::initServer(ENetAddress& address) {
    std::lock_guard<std::mutex> lock(m_hostMtx);
    if (enet_initialize () != 0) {
        LOG("Failed to initialize");
        return false;
    }

    /* Bind the server to the default localhost.     */
    /* A specific host address can be specified by   */
    /* enet_address_set_host (& address, "x.x.x.x"); */
    address.host = ENET_HOST_ANY; // This allows
    // Bind the server to port 5555
    address.port = 5555;

    const int MAX_PLAYERS = 32;

    m_host = enet_host_create (&address,    // the address to bind the server host to
                    MAX_PLAYERS,  // allow up to 32 clients and/or outgoing connections
                    2,  // allow up to 2 channels to be used, 0 and 1
                    0,  // assume any amount of incoming bandwidth
                    0); // assume any amount of outgoing bandwidth

    if (m_host == NULL) {
        LOG("Failed to create host");
        return false;
    }

    return true;
}

void ServerNetworking::receivePacket(ENetPacket* packet, ENetPeer* peer, ServerWorld<false>& mainWorld) {
    Packet<int, 0> head;
    memcpy(&head, packet->data, head.getSize());
    switch (head.getPacketType()) {
    case PacketType::ClientConnection:
    {
        // Add the player to the world
        Packet<int, 1> payload;
        memcpy(&payload, packet->data, packet->dataLength);
        int blockPosition[3] =  { 0, 0, 0 };
        float subBlockPosition[3] = { 0.0f, 0.0f, 0.0f };
        uint16_t playerID = mainWorld.addPlayer(blockPosition, subBlockPosition, payload[0], peer);
        // Send a response
        Packet<uint16_t, 1> responsePayload(0, PacketType::ClientConnection, 1);
        responsePayload[0] = playerID;
        m_hostMtx.lock();
        ENetPacket* response = enet_packet_create((const void*)(&responsePayload), responsePayload.getSize(), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(mainWorld.getPlayer(playerID).getPeer(), 0, response);
        m_hostMtx.unlock();
    }
    break;
    case PacketType::ClientPosition:
    {
        Packet<int64_t, 6> payload;
        memcpy(&payload, packet->data, packet->dataLength);
        uint16_t playerID = payload.getPeerID();
        auto it = mainWorld.getPlayers().find(playerID);
        if (it == mainWorld.getPlayers().end()) {

        }
        else {
            it->second.packetReceived(mainWorld.getTickNum());
            int oldPlayerCoords[3];
            it->second.getBlockPosition(oldPlayerCoords);
            IVec3 oldPlayerChunkCoords = Chunk::getChunkCoords(oldPlayerCoords);
            int newPlayerPos[3] = {
                        static_cast<int>(payload[0]),
                        static_cast<int>(payload[1]),
                        static_cast<int>(payload[2])
            };
            IVec3 newPlayerChunkCoords = Chunk::getChunkCoords(newPlayerPos);
            bool unloadNeeded = newPlayerChunkCoords != oldPlayerChunkCoords;
            if (unloadNeeded) {
                mainWorld.pauseChunkLoaderThreads();
            }

            float subBlockPosition[3] = { 0.0f, 0.0f, 0.0f };
            mainWorld.updatePlayerPos(playerID, newPlayerPos, subBlockPosition, unloadNeeded);
            mainWorld.setPlayerChunkLoadingTarget(playerID, payload[3], payload[4], payload[5]);

            if (unloadNeeded) {
                mainWorld.releaseChunkLoaderThreads();
            }
        }
    }
    break;
    case PacketType::BlockReplaced:
    {
        Packet<int, 4> payload;
        memcpy(&payload, packet->data, packet->dataLength);
        int blockCoords[3];
        memcpy(blockCoords, payload.getPayloadAddress(), 3 * sizeof(int));
        mainWorld.chunkManager.setBlock(blockCoords, payload[3]);
        mainWorld.broadcastBlockReplaced(blockCoords, payload[3], payload.getPeerID());
    }
    break;
    case PacketType::ChunkRequest:
    {
        Packet<int64_t, 2> payload;
        memcpy(&payload, packet->data, packet->dataLength);
        LOG("Chunk request for " + std::to_string(payload[1]));
        uint16_t playerID = payload.getPeerID();
        auto it = mainWorld.getPlayers().find(playerID);
        if (it == mainWorld.getPlayers().end()) {

        }
        else {
            mainWorld.setPlayerChunkLoadingTarget(playerID, payload[0], payload[1], payload[2]);
        }
    }
    break;

    default:
    break;
    }
    m_hostMtx.lock();
    enet_packet_destroy(packet);
    m_hostMtx.unlock();
}

void ServerNetworking::receiveEvents(ServerWorld<false>& mainWorld) {
    ENetEvent event;
    m_hostMtx.lock();
    while (enet_host_service(m_host, &event, 0) > 0) {
        m_hostMtx.unlock();
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                LOG("A new client connected from "
                    + std::to_string(event.peer->address.host) + ":"
                    + std::to_string(event.peer->address.port));
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                receivePacket(event.packet, event.peer, mainWorld);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                if (mainWorld.getPlayers().contains(event.data))
                    mainWorld.disconnectPlayer(event.data);
                // Reset the peer's client information
                event.peer->data = NULL;
                break;
            case ENET_EVENT_TYPE_NONE:
                break;
        }
        m_hostMtx.lock();
    }
    m_hostMtx.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(4));
}

}  // namespace lonelycube::server
