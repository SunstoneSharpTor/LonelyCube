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

#include "client/input.h"

static std::vector<int> buttonsDown;
static std::array<std::vector<int>, 2> pressedButtons;
static std::size_t pressedButtonsIndex = 0;
static std::string testText;

namespace lonelycube::client::input {

void characterCallback(GLFWwindow* window, unsigned int codepoint)
{
    testText = testText + (char)codepoint;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (std::count(buttonsDown.begin(), buttonsDown.end(), scancode) == 0)
            buttonsDown.push_back(scancode);

        bool notPressed = std::count(
            pressedButtons[!pressedButtonsIndex].begin(),
            pressedButtons[!pressedButtonsIndex].end(), scancode
        ) == 0;
        if (notPressed)
            pressedButtons[!pressedButtonsIndex].push_back(scancode);
    }
    else if (action == GLFW_RELEASE)
    {
        auto itr = std::find(buttonsDown.begin(), buttonsDown.end(), scancode);
        if (itr != buttonsDown.end())
        {
            buttonsDown.erase(itr);
        }
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    keyCallback(window, 0, -2 - button, action, mods);
}

void swapBuffers()
{
    pressedButtons[pressedButtonsIndex].clear();
    pressedButtonsIndex = !pressedButtonsIndex;
}

void clearCurrentBuffer()
{
    pressedButtons[pressedButtonsIndex].clear();
}

bool buttonPressed(int scancode)
{
    return std::count(
        pressedButtons[pressedButtonsIndex].begin(), pressedButtons[pressedButtonsIndex].end(),
        scancode
    );
    // auto itr = std::find(
    //     pressedButtons[pressedButtonsIndex].begin(), pressedButtons[pressedButtonsIndex].end(),
    //     scancode
    // );
    // if (itr == pressedButtons[pressedButtonsIndex].end())
    //     return false;
    //
    // pressedButtons[pressedButtonsIndex].erase(itr);
    // return true;
}

bool buttonDown(int scancode)
{
    return std::count(buttonsDown.begin(), buttonsDown.end(), scancode);
}

bool leftMouseButtonPressed()
{
    return buttonPressed(-2 - GLFW_MOUSE_BUTTON_LEFT);
}

bool anyMouseButtonPressed()
{
    for (int i = -2 - 0; i > -2 - 8; i--)
    {
        if (buttonPressed(i))
            return true;
    }
    return false;
}

}  // namespace lonelycube::client
