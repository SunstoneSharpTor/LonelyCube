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
class ECSView
{
private:
    ECS& ecs;
    ComponentMask componentMask;
    bool all{ false };

    class Iterator
    {
    private:
        bool validIndex() const;

    public:
        EntityIndex index;
        ECS& ecs;
        ComponentMask mask;
        bool all{ false };

        Iterator(ECS& ecs, EntityIndex index, ComponentMask mask, bool all);

        const Iterator end() const;

        EntityId operator*() const;

        bool operator==(const Iterator& other) const;

        bool operator!=(const Iterator& other) const;

        Iterator& operator++();
    };

public:
    ECSView(ECS& scene);

    const Iterator begin() const;

    const Iterator end() const;
};


template<typename... ComponentTypes>
ECSView<ComponentTypes...>::ECSView(ECS& ecs) : ecs(ecs)
{
    if (sizeof...(ComponentTypes) == 0)
    {
        all = true;
    }
    else
    {
        // Unpack the template parameters into an initializer list
        int componentIds[] = { 0, ecs.getComponentId<ComponentTypes>() ... };
        for (int i = 1; i < (sizeof...(ComponentTypes) + 1); i++)
            componentMask.set(componentIds[i]);
    }
}

template<typename... ComponentTypes>
ECSView<ComponentTypes...>::Iterator::Iterator(ECS& ecs, EntityIndex index, ComponentMask mask,
    bool all) : ecs(ecs), index(index), mask(mask), all(all) {}

template<typename... ComponentTypes>
EntityId ECSView<ComponentTypes...>::Iterator::operator*() const
{
    return ecs.getEntityId(index);
}

template<typename... ComponentTypes>
bool ECSView<ComponentTypes...>::Iterator::operator==(const Iterator& other) const
{
    return index == other.index;
}

template<typename... ComponentTypes>
bool ECSView<ComponentTypes...>::Iterator::operator!=(const Iterator& other) const
{
    return index != other.index;
}

template<typename... ComponentTypes>
bool ECSView<ComponentTypes...>::Iterator::validIndex() const
{
    return ecs.isEntityValid(ecs.getEntityId(index)) && (all || mask ==
        (mask & ecs.getEntityComponentMask(index)));
}

template<typename... ComponentTypes>
ECSView<ComponentTypes...>::Iterator& ECSView<ComponentTypes...>::Iterator::operator++() 
{
    do
    {
        index++;
    } while (index < ecs.getSize() && !validIndex());
    return *this;
}

template<typename... ComponentTypes>
const ECSView<ComponentTypes...>::Iterator ECSView<ComponentTypes...>::begin() const
{
    int firstIndex = 0;
    while (firstIndex < ecs.getSize()
        && (componentMask != (componentMask & ecs.getEntityComponentMask(firstIndex)))
        || !ecs.isEntityValid(ecs.getEntityId(firstIndex)))
    {
        firstIndex++;
    }
    return Iterator(ecs, firstIndex, componentMask, all);
}

template<typename... ComponentTypes>
const ECSView<ComponentTypes...>::Iterator ECSView<ComponentTypes...>::end() const
{
    return Iterator(ecs, EntityIndex(ecs.getSize()), componentMask, all);
}
