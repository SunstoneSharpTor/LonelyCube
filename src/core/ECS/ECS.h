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

#include "core/pch.h"
#include <bitset>

typedef uint64_t EntityID;
const int MAX_COMPONENTS = 32;
typedef std::bitset<MAX_COMPONENTS> ComponentMask;

struct EntityDesc
{
    EntityID id;
    ComponentMask mask;
};


struct ComponentPool
{
    std::unique_ptr<char[]> pData;
    size_t elementSize;
    ComponentPool(size_t elementsize, int maxEntities) : elementSize(elementsize),
        pData(std::make_unique<char[]>(elementSize * maxEntities)) {}

    inline void* get(size_t index)
    {
        return pData.get() + index * elementSize;
    }
};


class ECS {
private: 
    inline static int m_componentCounter = 0;
    template<typename T>
    inline static int s_componentID = m_componentCounter++;

    int m_maxEntities;
    std::vector<EntityDesc> m_entities;
    std::vector<std::unique_ptr<ComponentPool>> m_componentPools;

public:
    ECS(int maxEntities);

    template<typename T>
    static int getID();

    EntityID newEntity();

    template<typename T>
    T& assign(EntityID id);

    template<typename T>
    T& get(EntityID id);
};


ECS::ECS(int maxEntities) : m_maxEntities(maxEntities) {}

template<typename T>
int ECS::getID() 
{
    return s_componentID<T>;
}

EntityID ECS::newEntity()
{
    m_entities.push_back({ m_entities.size(), ComponentMask() });
    return m_entities.back().id;
}

template<typename T>
T& ECS::assign(EntityID id)
{
    int componentID = getID<T>();

    if (m_componentPools.size() <= componentID) // New component, make a new pool
    {
        m_componentPools.resize(componentID + 1);
        m_componentPools[componentID] = std::make_unique<ComponentPool>(sizeof(T), m_maxEntities);
    }

    // Looks up the component in the pool, and initializes it with placement new
    T* component = new (m_componentPools[componentID]->get(id)) T();

    // Set the bit for this component to true and return the created component
    m_entities[id].mask.set(componentID);

    return *component;
}

template<typename T>
T& ECS::get(EntityID id)
{
    int componentID = getID<T>();
    return *static_cast<T*>(m_componentPools[componentID]->get(id));
}