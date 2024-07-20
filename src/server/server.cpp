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

#include "core/pch.h"
#include <cstring>

#include <enet/enet.h>

#include "core/chunk.h"
#include "core/packet.h"
#include "core/serverWorld.h"

void receiveCommands(bool* running) {
    std::string command;
    while (*running) {
        std::getline(std::cin, command);
        if (command == "quit") {
            *running = false;
        }
    }
}

void receivePacket(ENetPacket* packet, ENetPeer* peer, ServerWorld& mainWorld) {
    Packet<int, 0> head;
    memcpy(&head, packet->data, head.getSize());
    switch (head.getPacketType()) {
    case PacketType::ClientConnection:
    {
        // Add the player to the world
        Packet<int, 1> payload;
        memcpy(&payload, packet->data, packet->dataLength);
        int blockPosition[3] =  { 0, 200, 0 };
        float subBlockPosition[3] = { 0.0f, 0.0f, 0.0f };
        int playerID = mainWorld.addPlayer(blockPosition, subBlockPosition, payload[0], peer);
        // Send a response
        Packet<unsigned short, 1> responsePayload(0, PacketType::ClientConnection, 1);
        responsePayload[0] = playerID;
        ENetPacket* response = enet_packet_create((const void*)(&responsePayload), responsePayload.getSize(), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(mainWorld.getPlayer(playerID).getPeer(), 0, response);
    }
    break;
    
    default:
        break;
    }
}

void chunkLoaderThread(ServerWorld* mainWorld, bool* running, char threadNum) {
    while (*running) {
        Position chunkPosition;
        if (mainWorld->loadChunk(&chunkPosition)) {

        }
    }
}

int main (int argc, char** argv) {
    if (enet_initialize () != 0) {
        return EXIT_FAILURE;
    }

    ENetEvent event;
    ENetAddress address;
    ENetHost* server;

    /* Bind the server to the default localhost.     */
    /* A specific host address can be specified by   */
    /* enet_address_set_host (& address, "x.x.x.x"); */
    address.host = ENET_HOST_ANY; // This allows
    // Bind the server to port 7777
    address.port = 5555;

    const int MAX_PLAYERS = 32;

    server = enet_host_create (&address,    // the address to bind the server host to
                    MAX_PLAYERS,  // allow up to 32 clients and/or outgoing connections
                    1,  // allow up to 1 channel to be used, 0
                    0,  // assume any amount of incoming bandwidth
                    0); // assume any amount of outgoing bandwidth

    if (server == NULL) {
        return 1;
    }

    unsigned int worldSeed = std::time(0);
    ServerWorld mainWorld(false, worldSeed);
    std::cout << "World Seed: " << worldSeed << std::endl;

    unsigned char numChunkLoaderThreads = std::max(1u, std::min(8u, std::thread::hardware_concurrency()));
    bool* chunkLoaderThreadsRunning = new bool[numChunkLoaderThreads];
    std::fill(chunkLoaderThreadsRunning, chunkLoaderThreadsRunning + numChunkLoaderThreads, true);

    bool running = true;

    std::thread* chunkLoaderThreads = new std::thread[numChunkLoaderThreads];
    for (char threadNum = 0; threadNum < numChunkLoaderThreads; threadNum++) {
        chunkLoaderThreads[threadNum] = std::thread(chunkLoaderThread, &mainWorld, &running, threadNum);
    }

    // Gameloop
    std::thread(receiveCommands, &running).detach();
    while(running) {
        ENetEvent event;
        while (enet_host_service (server, & event, 1000) > 0) {
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

    chunkLoaderThreadsRunning[0] = false;

    for (char threadNum = 0; threadNum < numChunkLoaderThreads; threadNum++) {
        chunkLoaderThreads[threadNum].join();
        chunkLoaderThreadsRunning[threadNum] = false;
    }

    enet_host_destroy(server);

    enet_deinitialize();

    return 0;
}