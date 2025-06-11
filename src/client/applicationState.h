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

namespace lonelycube::client {

class ApplicationState
{
public:
    enum State
    {
        StartMenu, SingleplayerMenu, MultiplayerMenu, Gameplay, PauseMenu
    };

    ApplicationState();
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
    inline const bool isDebugInfoEnabled()
    {
        return m_showDebugInfo;
    }
    inline void toggleDebugInfo()
    {
        m_showDebugInfo = !m_showDebugInfo;
    }

private:
    std::vector<State> m_state;
    bool m_showDebugInfo;
};

}  // namespace lonelycube::client
