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

namespace lonelycube::client {

class Menu
{
public:
    enum ElementType
    {
        Button
    };

    struct Element
    {
        ElementType type;
        union
        {
            struct
            {
                bool mouseOver;
                int width;
                glm::vec2 screenAlignment;
                glm::ivec2 pixelOffset;
            } buttonData;
        };
        std::string text;
    };

    Menu(const glm::ivec2 windowDimensions);
    void update(glm::ivec2 cursorPos);
    void resize(const glm::ivec2 windowDimensions);
    std::size_t addButton(
        int width, glm::vec2 screenAlignment, glm::ivec2 offset, const std::string& text
    );

    inline void setScale(int scale)
    {
        m_scale = scale;
    }
    inline int getScale() const
    {
        return m_scale;
    }
    inline const std::vector<Element>& getElements() const
    {
        return m_elements;
    }
    inline const Element& getElement(std::size_t index) const
    {
        return m_elements[index];
    }

private:
    int m_scale;
    glm::vec2 m_windowDimensions;
    std::vector<Element> m_elements;
};

}  // namespace lonelycube::client
