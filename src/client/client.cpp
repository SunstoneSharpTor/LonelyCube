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
#include "core/threadManager.h"

using namespace client;

void chunkLoaderThreadSingleplayer(ClientWorld& mainWorld, bool& running, int8_t threadNum, int&
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

void chunkLoaderThreadMultiplayer(ClientWorld& mainWorld, ClientNetworking& networking, bool&
    running, int8_t threadNum, int& numThreadsBeingUsed) {
    while (running) {
        while (threadNum >= numThreadsBeingUsed && running) {
            mainWorld.setThreadWaiting(threadNum, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
            mainWorld.setThreadWaiting(threadNum, false);
        }
        mainWorld.loadChunksAroundPlayerMultiplayer(threadNum);
        networking.receiveEvents(mainWorld);
    }
}

void manageThreads(int numChunkLoaderThreads, int& numThreadsBeingUsed) {

}

int main(int argc, char* argv[]) {
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
    ClientWorld mainWorld(settings.getRenderDistance(), worldSeed, !multiplayer, playerSpawnPoint);
    std::cout << "World Seed: " << worldSeed << std::endl;
    ClientPlayer mainPlayer(playerSpawnPoint, &mainWorld, mainWorld.getResourcePack());

    bool running = true;
    int frameTime;

    bool* chunkLoaderThreadsRunning = new bool[mainWorld.getNumChunkLoaderThreads()];
    std::fill(chunkLoaderThreadsRunning, chunkLoaderThreadsRunning + mainWorld.getNumChunkLoaderThreads(), true);

    RenderThread renderThread(&mainWorld, chunkLoaderThreadsRunning, &mainPlayer, networking, &frameTime);

    std::thread renderWorker(&RenderThread::go, renderThread, &running);
    
    ThreadManager threadManager(mainWorld.getNumChunkLoaderThreads() - 1);
    for (int8_t threadNum = 0; threadNum < threadManager.getNumThreads(); threadNum++) {
        if (multiplayer) {
            threadManager.getThread(threadNum) = std::thread(chunkLoaderThreadMultiplayer,
                std::ref(mainWorld), std::ref(networking), std::ref(running), threadNum + 1,
                std::ref(threadManager.getNumThreadsBeingUsed()));
        }
        else {
            threadManager.getThread(threadNum) = std::thread(chunkLoaderThreadSingleplayer,
                std::ref(mainWorld), std::ref(running), threadNum + 1,
                std::ref(threadManager.getNumThreadsBeingUsed()));
        }
    }

    if (multiplayer) {
        auto nextTick = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
        while (running) {
            mainWorld.loadChunksAroundPlayerMultiplayer(0);

            auto currentTime = std::chrono::steady_clock::now();
            if (currentTime >= nextTick) {
                //manageThreads(mainWorld.getNumChunkLoaderThreads(), numThreadsBeingUsed);
                
                Packet<int, 3> payload(mainWorld.getClientID(), PacketType::ClientPosition, 3);
                payload[0] = mainPlayer.cameraBlockPosition[0];
                payload[1] = mainPlayer.cameraBlockPosition[1];
                payload[2] = mainPlayer.cameraBlockPosition[2];
                ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
                enet_peer_send(networking.getPeer(), 0, packet);

                nextTick += std::chrono::milliseconds(100);
            }
        }
    }
    else {
        auto nextTick = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
        while (running) {
            mainWorld.loadChunksAroundPlayerSingleplayer(0);

            auto currentTime = std::chrono::steady_clock::now();
            if (currentTime >= nextTick) {
                //manageThreads(mainWorld.getNumChunkLoaderThreads(), numThreadsBeingUsed);
                mainWorld.tick();
                nextTick += std::chrono::milliseconds(100);
            }
        }
    }
    chunkLoaderThreadsRunning[0] = false;

    threadManager.joinThreads();
    for (int8_t threadNum = 1; threadNum < mainWorld.getNumChunkLoaderThreads(); threadNum++) {
        chunkLoaderThreadsRunning[threadNum] = false;
    }
    renderWorker.join();

    delete chunkLoaderThreadsRunning;

    if (multiplayer) {
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