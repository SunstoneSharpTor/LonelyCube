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

#include "serverNetworking.h"

#include "core/pch.h"
#include <cstring>

#include "core/packet.h"
#include "core/serverWorld.h"

namespace server {

bool ServerNetworking::initServer(ENetAddress& address) {
    if (enet_initialize () != 0) {
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
                    1,  // allow up to 1 channel to be used, 0
                    0,  // assume any amount of incoming bandwidth
                    0); // assume any amount of outgoing bandwidth

    if (m_host == NULL) {
        return false;
    }

    return true;
}

void ServerNetworking::receivePacket(ENetPacket* packet, ENetPeer* peer, ServerWorld& mainWorld) {
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
        unsigned short playerID = mainWorld.addPlayer(blockPosition, subBlockPosition, payload[0], peer);
        // Send a response
        Packet<unsigned short, 1> responsePayload(0, PacketType::ClientConnection, 1);
        responsePayload[0] = playerID;
        ENetPacket* response = enet_packet_create((const void*)(&responsePayload), responsePayload.getSize(), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(mainWorld.getPlayer(playerID).getPeer(), 0, response);
    }
    case PacketType::ClientPosition:
    {
        Packet<int, 3> payload;
        memcpy(&payload, packet->data, packet->dataLength);
        unsigned short playerID = payload.getPeerID();
        auto it = mainWorld.getPlayers().find(playerID);
        if (it == mainWorld.getPlayers().end()) {
            
        }
        else {
            it->second.packetReceived(mainWorld.getTickNum());
            int oldPlayerChunkPosition[3];
            it->second.getBlockPosition(oldPlayerChunkPosition);
            oldPlayerChunkPosition[0] = std::floor((float)oldPlayerChunkPosition[0] / constants::CHUNK_SIZE);
            oldPlayerChunkPosition[1] = std::floor((float)oldPlayerChunkPosition[1] / constants::CHUNK_SIZE);
            oldPlayerChunkPosition[2] = std::floor((float)oldPlayerChunkPosition[2] / constants::CHUNK_SIZE);
            int newPlayerPos[3];
            memcpy(newPlayerPos, payload.getPayloadAddress(), 3 * sizeof(int));
            int newPlayerChunkPosition[3];
            newPlayerChunkPosition[0] = std::floor((float)newPlayerPos[0] / constants::CHUNK_SIZE);
            newPlayerChunkPosition[1] = std::floor((float)newPlayerPos[1] / constants::CHUNK_SIZE);
            newPlayerChunkPosition[2] = std::floor((float)newPlayerPos[2] / constants::CHUNK_SIZE);
            bool unloadNeeded = !((oldPlayerChunkPosition[0] == newPlayerChunkPosition[0])
                && (oldPlayerChunkPosition[1] == newPlayerChunkPosition[1])
                && (oldPlayerChunkPosition[2] == newPlayerChunkPosition[2]));
            if (unloadNeeded) {
                mainWorld.pauseChunkLoaderThreads();
                std::cout << "UNLOAD\n";
            }

            float subBlockPosition[3] = { 0.0f, 0.0f, 0.0f };
            mainWorld.updatePlayerPos(playerID, newPlayerPos, subBlockPosition, unloadNeeded);

            if (unloadNeeded) {
                mainWorld.releaseChunkLoaderThreads();
            }
        }
    }
    break;
    
    default:
        break;
    }
}

void ServerNetworking::receiveEvents(ServerWorld& mainWorld) {
    ENetEvent event;
    while (enet_host_service (m_host, &event, 5) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "A new client connected from "
                    << event.peer -> address.host << ":"
                    << event.peer -> address.port << "\n";
            break;

            case ENET_EVENT_TYPE_RECEIVE:
                receivePacket(event.packet, event.peer, mainWorld);
            break;

            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << event.peer->data << " disconnected.\n";
                // Reset the peer's client information
                event.peer->data = NULL;
        }
    }
}

}  // namespace server