/*
  Lonely Cube, a voxel game
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
#include "server/serverNetworking.h"
#include "core/resourceMonitor.h"

using namespace server;

static void receiveCommands(bool* running) {
    std::string command;
    while (*running) {
        std::getline(std::cin, command);
        if (command == "quit") {
            *running = false;
        }
    }
}

static void chunkLoaderThread(ServerWorld<false>* mainWorld, bool* running, int8_t threadNum) {
    while (*running) {
        mainWorld->waitIfRequired(threadNum);
        IVec3 chunkPosition;
        if (mainWorld->loadNextChunk(&chunkPosition)) {

        }
    }
}

int main (int argc, char** argv) {
    // enet_initialize();
    // ENetAddress address;
    // ENetEvent event;
    // ENetHost * server;
    //
    // /* Bind the server to the default localhost.     */
    // /* A specific host address can be specified by   */
    // /* enet_address_set_host (& address, "x.x.x.x"); */
    //
    // address.host = ENET_HOST_ANY;
    // /* Bind the server to port 1234. */
    // address.port = 5555;
    //
    // server = enet_host_create (& address /* the address to bind the server host to */, 
    //                              32      /* allow up to 32 clients and/or outgoing connections */,
    //                               2      /* allow up to 2 channels to be used, 0 and 1 */,
    //                               0      /* assume any amount of incoming bandwidth */,
    //                               0      /* assume any amount of outgoing bandwidth */);
    // if (server == NULL)
    // {
    //     fprintf (stderr,
    //              "An error occurred while trying to create an ENet server host.\n");
    //     exit (EXIT_FAILURE);
    // }
    //
    // std::vector<ENetPeer*> peers;
    // int numPackets = 0;
    // auto nextSend = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
    //
    // while (true)
    // {
    //     while (enet_host_service (server, & event, 0) > 0)
    //     {
    //         switch (event.type)
    //         {
    //         case ENET_EVENT_TYPE_CONNECT:
    //             printf ("A new client connected from %x:%u.\n", 
    //                     event.peer -> address.host,
    //                     event.peer -> address.port);
    //
    //             /* Store any relevant client information here. */
    //             event.peer -> data = (void*)"Client information";
    //             peers.push_back(event.peer);
    //
    //             break;
    //
    //         case ENET_EVENT_TYPE_RECEIVE:
    //             printf ("A packet of length %u containing %s was received from %s on channel %u.\n",
    //                     event.packet -> dataLength,
    //                     event.packet -> data,
    //                     event.peer -> data,
    //                     event.channelID);
    //             std::cout << numPackets++ << std::endl;
    //
    //             /* Clean up the packet now that we're done using it. */
    //             enet_packet_destroy (event.packet);
    //
    //             break;
    //
    //         case ENET_EVENT_TYPE_DISCONNECT:
    //             printf ("%s disconnected.\n", event.peer -> data);
    //
    //             /* Reset the peer's client information. */
    //
    //             event.peer -> data = NULL;
    //         }
    //     }
    //
    //     if (std::chrono::steady_clock::now() > nextSend)
    //     {
    //         nextSend += std::chrono::milliseconds(50);
    //         /* Create a reliable packet of size 7 containing "packet\0" */
    //         ENetPacket * packet = enet_packet_create ("packet", 
    //                                                   strlen ("packet") + 1, 
    //                                                   ENET_PACKET_FLAG_RELIABLE);
    //
    //         /* Send the packet to the peer over channel id 0. */
    //         /* One could also broadcast the packet by         */
    //         /* enet_host_broadcast (host, 0, packet);         */
    //         for (ENetPeer* peer : peers)
    //             enet_peer_send (peer, 0, packet);
    //     }
    // }
    //
    // enet_host_destroy(server);
    // enet_deinitialize();
    //
    // return 0;



    ENetEvent event;
    ENetAddress address;
    ServerNetworking networking;

    if (!networking.initServer(address)) {
        std::cout << "Failed to initialise server" << std::endl;
        return 0;
    }

    uint32_t worldSeed = std::time(0);
    ServerWorld<false> mainWorld(worldSeed, networking.getMutex());
    std::cout << "World Seed: " << worldSeed << std::endl;

    uint8_t numChunkLoaderThreads = mainWorld.getNumChunkLoaderThreads();
    bool* chunkLoaderThreadsRunning = new bool[numChunkLoaderThreads];
    std::fill(chunkLoaderThreadsRunning, chunkLoaderThreadsRunning + numChunkLoaderThreads, true);

    bool running = true;

    std::thread* chunkLoaderThreads = new std::thread[numChunkLoaderThreads];
    for (int8_t threadNum = 0; threadNum < numChunkLoaderThreads; threadNum++) {
        chunkLoaderThreads[threadNum] = std::thread(chunkLoaderThread, &mainWorld, &running, threadNum);
    }

    // Gameloop
    std::thread(receiveCommands, &running).detach();
    auto nextTick = std::chrono::steady_clock::now() + std::chrono::nanoseconds(1000000000 / constants::TICKS_PER_SECOND);
    while(running) {
        auto currentTime = std::chrono::steady_clock::now();
        if (currentTime >= nextTick) {
            mainWorld.tick();
            nextTick += std::chrono::nanoseconds(1000000000 / constants::TICKS_PER_SECOND);
        }

        networking.receiveEvents(mainWorld);
    }

    chunkLoaderThreadsRunning[0] = false;

    for (int8_t threadNum = 0; threadNum < numChunkLoaderThreads; threadNum++) {
        chunkLoaderThreads[threadNum].join();
        chunkLoaderThreadsRunning[threadNum] = false;
    }

    enet_host_destroy(networking.getHost());

    enet_deinitialize();

    return 0;
}
