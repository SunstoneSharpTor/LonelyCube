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

#include "core/pch.h"
#include <time.h>

#include "client/clientNetworking.h"
#include "client/renderThread.h"
#include "client/clientWorld.h"
#include "client/clientPlayer.h"
#include "core/config.h"
#include "core/packet.h"

using namespace client;

void chunkLoaderThreadSingleplayer(ClientWorld& mainWorld, bool* running, char threadNum) {
    while (*running) {
        mainWorld.loadChunksAroundPlayerSingleplayer(threadNum);
    }
}

void chunkLoaderThreadMultiplayer(ClientWorld& mainWorld, ClientNetworking& networking, bool* running, char threadNum) {
    while (*running) {
        mainWorld.loadChunksAroundPlayerMultiplayer(threadNum);
        networking.receiveEvents(mainWorld);
    }
}

int main(int argc, char* argv[]) {
    bool MULTIPLAYER = true;

    Config settings("res/settings.txt");
    
    ClientNetworking networking;

    if (MULTIPLAYER) {
        if (!networking.establishConnection(settings.getServerIP(), settings.getRenderDistance())) {
            MULTIPLAYER = false;
        }
    }

    unsigned int worldSeed = std::time(0);
    int playerSpawnPoint[3] = { 0, 200, 0 };
    ClientWorld mainWorld(settings.getRenderDistance(), worldSeed, !MULTIPLAYER, playerSpawnPoint);
    std::cout << "World Seed: " << worldSeed << std::endl;
    ClientPlayer mainPlayer(playerSpawnPoint, &mainWorld);

    bool running = true;

    bool* chunkLoaderThreadsRunning = new bool[mainWorld.getNumChunkLoaderThreads()];
    std::fill(chunkLoaderThreadsRunning, chunkLoaderThreadsRunning + mainWorld.getNumChunkLoaderThreads(), true);

    RenderThread renderThread(&mainWorld, chunkLoaderThreadsRunning, &mainPlayer, networking);

    std::thread renderWorker(&RenderThread::go, renderThread, &running);

    std::thread* chunkLoaderThreads = new std::thread[mainWorld.getNumChunkLoaderThreads() - 1];
    for (char threadNum = 1; threadNum < mainWorld.getNumChunkLoaderThreads(); threadNum++) {
        if (MULTIPLAYER) {
            chunkLoaderThreads[threadNum - 1] = std::thread(chunkLoaderThreadMultiplayer, std::ref(mainWorld), std::ref(networking), &running, threadNum);
        }
        else {
            chunkLoaderThreads[threadNum - 1] = std::thread(chunkLoaderThreadSingleplayer, std::ref(mainWorld), &running, threadNum);
        }
    }

    if (MULTIPLAYER) {
        auto lastMessage = std::chrono::steady_clock::now();
        while (running) {
            mainWorld.loadChunksAroundPlayerMultiplayer(0);
            auto currentTime = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastMessage) > std::chrono::milliseconds(100)) {
                Packet<int, 3> payload(mainWorld.getClientID(), PacketType::ClientPosition, 3);
                payload[0] = mainPlayer.cameraBlockPosition[0];
                payload[1] = mainPlayer.cameraBlockPosition[1];
                payload[2] = mainPlayer.cameraBlockPosition[2];
                ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
                enet_peer_send(networking.getPeer(), 0, packet);
                lastMessage += std::chrono::milliseconds(100);
            }
        }
    }
    else {
        while (running) {
            mainWorld.loadChunksAroundPlayerSingleplayer(0);
        }
    }
    chunkLoaderThreadsRunning[0] = false;

    for (char threadNum = 1; threadNum < mainWorld.getNumChunkLoaderThreads(); threadNum++) {
        chunkLoaderThreads[threadNum - 1].join();
        chunkLoaderThreadsRunning[threadNum] = false;
    }
    renderWorker.join();

    delete chunkLoaderThreadsRunning;

    if (MULTIPLAYER) {
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