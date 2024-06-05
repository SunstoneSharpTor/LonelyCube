#pragma once

#include <SDL2/SDL.h>

#include "world.h"
#include "camera.h"

class Player {
private:
    static const float m_hitBoxCorners[36];
    static const int m_directions[18];

    double m_time;
    double m_lastMousePoll;

    const Uint8* m_keyboardState;
    int m_lastMousePos[2];
    bool m_playing;
    bool m_lastPlaying;
    Uint32 m_pausedMouseState;

    float m_timeSinceBlockPlace;
    float m_timeSinceBlockBreak;
    float m_timeSinceLastJump;
    float m_timeSinceLastSpace;
    float m_timeSinceTouchGround;
    float m_timeSinceTouchWater;

    bool m_fly;
    bool m_crouch;
    bool m_lastSpace;

    float m_yaw;
    float m_pitch;

    bool m_touchGround;
    bool m_touchWater;

    glm::vec3 m_velocity;

    int m_hitboxMinBlock[3];
    glm::vec3 m_hitboxMinOffset;

    void resolveHitboxCollisions(float DT);

    bool collidingWithBlock();
    
    bool intersectingBlock(int* blockPos);
public:
    World* m_world;

	Camera viewCamera;
    int cameraBlockPosition[3];
    bool zoom;

    unsigned short m_blockHolding;

    Player(int* position, World* mainWorld);

    void setWorldMouseData(SDL_Window* window, int* windowDimensions);

    void processUserInput(SDL_Window* sdl_window, int* windowDimensions, bool* windowLastFocus, bool* running, double currentTime);
};