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

#pragma once

#include "core/ECS/ECS.h"

template<typename... ComponentTypes>
class SceneView
{
private:
    ECS& pScene;
    ComponentMask componentMask;
    bool all{ false };

    class Iterator
    {
    public:
        EntityIndex index;
        ECS& ecs;
        ComponentMask mask;
        bool all{ false };

        Iterator(ECS& ecs, EntityIndex index, ComponentMask mask, bool all);

        EntityId operator*() const;

        bool operator==(const Iterator& other) const;

        bool operator!=(const Iterator& other) const;

        Iterator& operator++();
    };
public:
    SceneView(ECS& scene);
};


template<typename... ComponentTypes>
SceneView<ComponentTypes...>::SceneView(ECS& scene) : pScene(scene)
{
    if (sizeof...(ComponentTypes) == 0)
    {
        all = true;
    }
    else
    {
        // Unpack the template parameters into an initializer list
        int componentIds[] = { 0, pScene.getId<ComponentTypes>() ... };
        for (int i = 1; i < (sizeof...(ComponentTypes) + 1); i++)
            componentMask.set(componentIds[i]);
    }
}

template<typename... ComponentTypes>
SceneView<ComponentTypes...>::Iterator::Iterator(ECS& ecs, EntityIndex index, ComponentMask mask,
    bool all) : m_ECS(ecs), m_index(index), m_mask(mask), m_all(all) {}

template<typename... ComponentTypes>
EntityId SceneView<ComponentTypes...>::Iterator::operator*() const
{
    return m_ECS.getEntityId(m_index);
}

template<typename... ComponentTypes>
bool SceneView<ComponentTypes...>::Iterator::operator==(const Iterator& other) const
{
    return 
