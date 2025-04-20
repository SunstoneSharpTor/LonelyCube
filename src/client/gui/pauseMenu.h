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

#pragma once

#include "client/gui/menu.h"
#include "client/gui/menuUpdateInfo.h"

namespace lonelycube::client {

class PauseMenu
{
public:
    PauseMenu(const glm::ivec2 windowDimensions);
    bool update(MenuUnpdateInfo& info);

    inline Menu& getMenu()
    {
        return m_menu;
    }

private:
    Menu m_menu;

    int m_backToGameButton;
    int m_leaveWorldButton;
};

}  // namespace lonelycube::client
