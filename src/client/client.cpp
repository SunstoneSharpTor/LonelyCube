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

#include <enet/enet.h>

#include "client/clientNetworking.h"
#include "client/renderThread.h"
#include "client/clientWorld.h"
#include "client/clientPlayer.h"
#include "core/config.h"
#include "core/constants.h"
#include "core/packet.h"
#include "core/threadManager.h"

using namespace client;

static void chunkLoaderThreadSingleplayer(ClientWorld& mainWorld, bool& running, int8_t threadNum, int&
    numThreadsBeingUsed) {
    while (running) {
        while (threadNum >= numThreadsBeingUsed && running) {
            mainWorld.setThreadWaiting(threadNum, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
            mainWorld.setThreadWaiting(threadNum, false);
        }
        mainWorld.loadChunksAroundPlayerSingleplayer(threadNum);
    }
}

static void chunkLoaderThreadMultiplayer(ClientWorld& mainWorld, ClientNetworking& networking, bool&
    running, int8_t threadNum, int& numThreadsBeingUsed) {
    int numNCalls = 0;
    while (running) {
        while (threadNum >= numThreadsBeingUsed && running) {
            mainWorld.setThreadWaiting(threadNum, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
            std::cout << numThreadsBeingUsed << std::endl;
            mainWorld.setThreadWaiting(threadNum, false);
        }
        // std::cout << numNCalls++ << std::endl;
        mainWorld.loadChunksAroundPlayerMultiplayer(threadNum);
        networking.receiveEvents(mainWorld);
    }
}

int main(int argc, char* argv[]) {
    // enet_initialize();
    // ENetHost * client;
    //
    // client = enet_host_create (NULL /* create a client host */,
    //             1 /* only allow 1 outgoing connection */,
    //             2 /* allow up 2 channels to be used, 0 and 1 */,
    //             0 /* assume any amount of incoming bandwidth */,
    //             0 /* assume any amount of outgoing bandwidth */);
    //
    // if (client == NULL)
    // {
    //     fprintf (stderr,
    //              "An error occurred while trying to create an ENet client host.\n");
    //     exit (EXIT_FAILURE);
    // }
    //
    // ENetAddress address;
    // ENetEvent event;
    // ENetPeer *peer;
    //
    // /* Connect to some.server.net:1234. */
    // enet_address_set_host (& address, "127.0.0.1");
    // address.port = 5555;
    //
    // /* Initiate the connection, allocating the two channels 0 and 1. */
    // peer = enet_host_connect (client, & address, 2, 0);
    //
    // if (peer == NULL)
    // {
    //    fprintf (stderr, 
    //             "No available peers for initiating an ENet connection.\n");
    //    exit (EXIT_FAILURE);
    // }
    //
    // /* Wait up to 5 seconds for the connection attempt to succeed. */
    // if (enet_host_service (client, & event, 5000) > 0 &&
    //     event.type == ENET_EVENT_TYPE_CONNECT)
    // {
    //     puts ("Connection to some.server.net:1234 succeeded.");
    // }
    // else
    // {
    //     /* Either the 5 seconds are up or a disconnect event was */
    //     /* received. Reset the peer in the event the 5 seconds   */
    //     /* had run out without any significant event.            */
    //     enet_peer_reset (peer);
    //
    //     puts ("Connection to some.server.net:1234 failed.");
    // }
    //
    // int numPackets = 0;
    // auto nextSend = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
    //
    // while (true)
    // {
    //     while (enet_host_service (client, & event, 0) > 0)
    //     {
    //         switch (event.type)
    //         {
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
    //         enet_peer_send (peer, 0, packet);
    //     }
    // }
    //
    // // Disconnect
    // enet_peer_disconnect (peer, 0);
    //
    // /* Allow up to 3 seconds for the disconnect to succeed
    //  * and drop any packets received packets.
    //  */
    // while (enet_host_service (client, & event, 3000) > 0)
    // {
    //     switch (event.type)
    //     {
    //     case ENET_EVENT_TYPE_RECEIVE:
    //         enet_packet_destroy (event.packet);
    //         break;
    //
    //     case ENET_EVENT_TYPE_DISCONNECT:
    //         puts ("Disconnection succeeded.");
    //         return 0;
    //     }
    // }
    //
    // /* We've arrived here, so the disconnect attempt didn't */
    // /* succeed yet.  Force the connection down.             */
    // enet_peer_reset (peer);
    // enet_host_destroy(client);
    //
    // return 0;



    Config settings("res/settings.txt");

    bool multiplayer = settings.getMultiplayer();

    ClientNetworking networking;

    if (multiplayer) {
        if (!networking.establishConnection(settings.getServerIP(), settings.getRenderDistance())) {
            multiplayer = false;
        }
    }

    uint32_t worldSeed = std::time(0);
    int playerSpawnPoint[3] = { 0, 200, 0 };
    ClientWorld mainWorld(
        settings.getRenderDistance(), worldSeed, !multiplayer, playerSpawnPoint,
        multiplayer ? networking.getPeer() : nullptr, networking.getMutex()
    );
    std::cout << "World Seed: " << worldSeed << std::endl;
    ClientPlayer mainPlayer(playerSpawnPoint, &mainWorld, mainWorld.integratedServer.getResourcePack());

    bool running = true;
    int frameTime;

    bool* chunkLoaderThreadsRunning = new bool[mainWorld.getNumChunkLoaderThreads()];
    std::fill(chunkLoaderThreadsRunning, chunkLoaderThreadsRunning + mainWorld.getNumChunkLoaderThreads(), true);

    RenderThread renderThread(&mainWorld, chunkLoaderThreadsRunning, &mainPlayer, networking, &frameTime);

    std::thread renderWorker(&RenderThread::go, renderThread, &running);

    ThreadManager threadManager(mainWorld.getNumChunkLoaderThreads());
    for (int8_t threadNum = 1; threadNum < threadManager.getNumThreads(); threadNum++) {
        if (multiplayer) {
            threadManager.getThread(threadNum) = std::thread(chunkLoaderThreadMultiplayer,
                std::ref(mainWorld), std::ref(networking), std::ref(running), threadNum,
                std::ref(threadManager.getNumThreadsBeingUsed()));
        }
        else {
            threadManager.getThread(threadNum) = std::thread(chunkLoaderThreadSingleplayer,
                std::ref(mainWorld), std::ref(running), threadNum,
                std::ref(threadManager.getNumThreadsBeingUsed()));
        }
    }

    if (multiplayer) {
        auto nextTick = std::chrono::steady_clock::now() + std::chrono::nanoseconds(1000000000 / constants::TICKS_PER_SECOND);
        while (running) {
            mainWorld.loadChunksAroundPlayerMultiplayer(0);
            networking.receiveEvents(mainWorld);

            auto currentTime = std::chrono::steady_clock::now();
            if (currentTime >= nextTick) {
                if (mainWorld.integratedServer.getTickNum() % 4 == 0)
                    threadManager.throttleThreads();

                // Send the server the player's position
                Packet<int, 3> payload(mainWorld.getClientID(), PacketType::ClientPosition, 3);
                payload[0] = mainPlayer.cameraBlockPosition[0];
                payload[1] = mainPlayer.cameraBlockPosition[1];
                payload[2] = mainPlayer.cameraBlockPosition[2];
                networking.getMutex().lock();
                ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), 0);
                enet_peer_send(networking.getPeer(), 1, packet);
                networking.getMutex().unlock();
                mainWorld.scheduleChunkRequest();

                nextTick += std::chrono::nanoseconds(1000000000 / constants::TICKS_PER_SECOND);
            }
        }
    }
    else {
        auto nextTick = std::chrono::steady_clock::now() + std::chrono::nanoseconds(1000000000 / constants::TICKS_PER_SECOND);
        while (running) {
            mainWorld.loadChunksAroundPlayerSingleplayer(0);

            auto currentTime = std::chrono::steady_clock::now();
            if (currentTime >= nextTick) {
                if (mainWorld.integratedServer.getTickNum() % 4 == 0)
                    threadManager.throttleThreads();
                mainWorld.integratedServer.tick();
                nextTick += std::chrono::nanoseconds(1000000000 / constants::TICKS_PER_SECOND);
            }
        }
    }
    chunkLoaderThreadsRunning[0] = false;

    threadManager.joinThreads();
    for (int8_t threadNum = 1; threadNum < mainWorld.getNumChunkLoaderThreads(); threadNum++) {
        chunkLoaderThreadsRunning[threadNum] = false;
    }
    renderWorker.join();

    delete[] chunkLoaderThreadsRunning;

    if (multiplayer) {
        std::lock_guard<std::mutex> lock(networking.getMutex());
        enet_peer_disconnect(networking.getPeer(), 0);
        ENetEvent event;
        while (enet_host_service(networking.getHost(), &event, 3000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
                case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "Disconnection succeeded!" << std::endl;
                break;
            }
        }

        enet_deinitialize();
    }

    return 0;
}
