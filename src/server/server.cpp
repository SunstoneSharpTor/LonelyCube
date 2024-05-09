#include <iostream>
#include <SDL2/SDL.h>
#include <enet/enet.h>

int main(int argc, char* argv[]) {
    if (enet_initialize() != 0) {
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetAddress address;
    ENetEvent event;
    ENetHost* server;

    address.host = ENET_HOST_ANY;
    address.port = 7777;

    const int MAX_PLAYERS = 32;

    server = enet_host_create(&address, MAX_PLAYERS, 1, 0, 0); 

    if (server == NULL) {
        return EXIT_FAILURE;
    }

    while (enet_host_service(server, &event, 1000) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
            //std::cout << 
            break;
        }
    }

    return 0;
}