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

#include "client/gui/startMenu.h"

#include "client/input.h"

namespace lonelycube::client {

StartMenu::StartMenu(const glm::ivec2 windowDimensions) 
    : m_menu(windowDimensions),
    m_singleplayerButton(m_menu.addButton(160, { 0.5f, 0.5f }, { -80, -7 }, "Singleplayer")),
    m_multiplayerButton(m_menu.addButton(160, { 0.5f, 0.5f }, { -80, 15 }, "Multiplayer")),
    m_quitGameButton(m_menu.addButton(160, { 0.5f, 0.5f }, { -80, 37 }, "Quit Game"))
{}

bool StartMenu::update(MenuUnpdateInfo& info)
{
    m_menu.setScale(info.guiScale);
    m_menu.resize(info.windowSize);
    m_menu.update(info.cursorPos);
    if (input::leftMouseButtonPressed())
    {
        if (m_menu.getElement(m_singleplayerButton).buttonData.mouseOver)
        {
            info.applicationState.pushState(ApplicationState::Gameplay);
            return true;
        }
        if (m_menu.getElement(m_quitGameButton).buttonData.mouseOver)
        {
            info.applicationState.popState();
            return true;
        }
    }

    return false;
}

}  // namespace lonelycube::client
