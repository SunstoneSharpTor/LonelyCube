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

const int MAX_COMPONENTS = 32;
typedef std::bitset<MAX_COMPONENTS> ComponentMask;
typedef uint32_t EntityIndex;
typedef uint32_t EntityVersion;
typedef uint64_t EntityId;


struct EntityDesc
{
    EntityId id;
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
    std::vector<EntityIndex> m_freeEntities;
    std::vector<ComponentPool*> m_componentPools;

public:
    ECS(int maxEntities);
    ~ECS();

    template<typename T>
    static int getId();

    EntityId newEntity();

    void destroyEntity(const EntityId id);

    template<typename T>
    T& assign(const EntityId id);

    template<typename T>
    void remove(const EntityId id);

    template<typename T>
    T& get(const EntityId id);

    inline bool isEntityAlive(const EntityId id)
    {
        return m_entities[getEntityIndex(id)].id == id;
    }

    inline static EntityId createEntityId(EntityIndex index, EntityVersion version)
    {
        return ((EntityId)index << 32) | ((EntityId)version);
    }

    inline static EntityIndex getEntityIndex(EntityId id)
    {
        return id >> 32;
    }

    inline static EntityVersion getEntityVersion(EntityId id)
    {
        return (EntityVersion)id;
    }

    inline static bool isEntityValid(EntityId id)
    {
        return (id >> 32) != EntityIndex(-1);
    }

private:
    inline static EntityId s_invalidEntity = createEntityId(EntityIndex(-1), 0);
};


ECS::ECS(int maxEntities) : m_maxEntities(maxEntities) {}

ECS::~ECS() {
    for (ComponentPool* pool : m_componentPools) {
        delete pool;
    }
}

template<typename T>
int ECS::getId() 
{
    return s_componentID<T>;
}

EntityId ECS::newEntity()
{
    if (m_freeEntities.empty())
    {
        EntityId id = createEntityId(m_entities.size(), 0);
        m_entities.emplace_back(id, ComponentMask());
        return id;
    }
    EntityIndex index = m_freeEntities.back();
    m_freeEntities.pop_back();
    EntityId id = createEntityId(index, getEntityVersion(m_entities[index].id));
    m_entities[index].id = id;
    return id;
}

void ECS::destroyEntity(const EntityId id)
{
    EntityId newId = createEntityId(EntityIndex(-1), getEntityVersion(id) + 1);
    m_entities[getEntityIndex(id)].id = newId;
    m_entities[getEntityIndex(id)].mask.reset();
    m_freeEntities.push_back(getEntityIndex(id));
}

template<typename T>
T& ECS::assign(const EntityId id)
{
    int componentId = getId<T>();

    if (m_componentPools.size() <= componentId) // New component, make a new pool
    {
        m_componentPools.resize(componentId + 1);
        m_componentPools[componentId] = new ComponentPool(sizeof(T), m_maxEntities);
    }

    // Looks up the component in the pool, and initializes it with placement new
    T* component = new (m_componentPools[componentId]->get(getEntityIndex(id))) T();

    // Set the bit for this component to true and return the created component
    m_entities[getEntityIndex(id)].mask.set(componentId);

    return *component;
}

template<typename T>
void ECS::remove(const EntityId id)
{
    int componentId = getId<T>();
    m_entities[getEntityIndex(id)].mask.reset(componentId);
}

template<typename T>
T& ECS::get(const EntityId id)
{
    int componentId = getId<T>();
    return *static_cast<T*>(m_componentPools[componentId]->get(getEntityIndex(id)));
}