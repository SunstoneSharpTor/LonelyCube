#include <iostream>
#include <string>
#include <thread>
#include <SDL2/SDL.h>
#include <enet/enet.h>

void receiveCommand(bool* quit) {
    std::string command;
    while (!(*quit)) {
        std::getline(std::cin, command);
        if (command == "quit") {
            *quit = true;
        }
    }
}

int main (int argc, char* argv) {
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
    address.port = 7777;

    const int MAX_PLAYERS = 32;

    server = enet_host_create (&address,    // the address to bind the server host to
                    MAX_PLAYERS,  // allow up to 32 clients and/or outgoing connections
                    1,  // allow up to 1 channel to be used, 0
                    0,  // assume any amount of incoming bandwidth
                    0); // assume any amount of outgoing bandwidth

    if (server == NULL) {
        return 1;
    }

    // Gameloop
    bool quit = false;
    std::thread(receiveCommand, &quit).detach();
    while(!quit) {
        ENetEvent event;
        // Wait up to 1000 milliseconds for an event
        while (enet_host_service (server, & event, 1000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    std::cout << "A new client connected from "
                        << event.peer -> address.host << ":"
                        << event.peer -> address.port << "\n";
                break;

                case ENET_EVENT_TYPE_RECEIVE:
                    std::cout << "A packet of length " << event.packet->dataLength
                        << " containing " << event.packet->data
                        << " was received from " << event.peer->data
                        << " on channel " << event.channelID << "\n";
                        // Clean up the packet now that we're done using it
                        enet_packet_destroy (event.packet);
                break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << event.peer->data << " disconnected.\n";
                    // Reset the peer's client information
                    event.peer->data = NULL;
            }
        }
    }

    enet_host_destroy(server);

    enet_deinitialize();

    return 0;
}