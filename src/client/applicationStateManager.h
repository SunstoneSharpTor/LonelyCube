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

#include "core/pch.h"

#include "glm/glm.hpp"

#include "client/gui/menu.h"

namespace lonelycube::client {

class ApplicationStateManager
{
public:
    enum State
    {
        StartMenu, SingleplayerMenu, MultiplayerMenu, Gameplay, PauseMenu
    };

    ApplicationStateManager();
    inline const std::vector<State>& getState()
    {
        return m_state;
    }
    inline void pushState(State newState)
    {
        m_state.push_back(newState);
    }
    inline void popState()
    {
        m_state.pop_back();
    }

private:
    std::vector<State> m_state;
};

}  // namespace lonelycube::client
