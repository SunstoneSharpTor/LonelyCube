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

#include "core/ECS/ECS.h"
#include "core/ECS/ECSView.h"
#include "core/position.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE( "Entities can be added, destroyed and have components assigned to them", "[ECS]" ) {
    ECS ecs(1000);
    EntityId entity1 = ecs.newEntity();
    Position& entity1Position = ecs.assign<Position>(entity1);
    entity1Position = { 0, 1, 0 };
    EntityId entity2 = ecs.newEntity();
    std::string& entity2String = ecs.assign<std::string>(entity2);
    entity2String = "Entity 2";
    std::string& entity1String = ecs.assign<std::string>(entity1);
    entity1String = "Entity 1";
    int& entity1Int = ecs.assign<int>(entity1);
    entity1Int = 1;
    Position& entity2Position = ecs.assign<Position>(entity2);
    entity2Position = { 0, 2, 0 };
    EntityId entity3 = ecs.newEntity();
    int& entity3Int = ecs.assign<int>(entity3);
    entity3Int = 3;

    SECTION( "Entities can be added" )
    {
        REQUIRE( ecs.get<Position>(entity1) == Position(0, 1, 0) );
        REQUIRE( entity1Position == Position(0, 1, 0) );
        REQUIRE( ecs.get<Position>(entity2) == Position(0, 2, 0) );
        REQUIRE( entity2Position == Position(0, 2, 0) );
        REQUIRE( ecs.get<std::string>(entity1) == "Entity 1" );
        REQUIRE( entity1String == "Entity 1" );
        REQUIRE( ecs.get<std::string>(entity2) == "Entity 2" );
        REQUIRE( entity2String == "Entity 2" );
        REQUIRE( ecs.get<int>(entity1) == 1 );
        REQUIRE( entity1Int == 1 );
        REQUIRE( ecs.get<int>(entity3) == 3 );
        REQUIRE( entity3Int == 3 );
    }
    SECTION( "Entities can be destroyed and have new entities added in their place" )
    {
        ecs.destroyEntity(entity2);
        EntityId entity4 = ecs.newEntity();
        int& entity4Int = ecs.assign<int>(entity4);
        entity4Int = 4;

        REQUIRE( ecs.get<int>(entity1) == 1 );
        REQUIRE( entity1Int == 1 );
        REQUIRE( ecs.get<int>(entity3) == 3 );
        REQUIRE( entity3Int == 3 );
        REQUIRE( ecs.get<int>(entity4) == 4 );
        REQUIRE( entity4Int == 4 );
        REQUIRE( ecs.getEntityIndex(entity4) == ecs.getEntityIndex(entity2) );
    }
}

TEST_CASE(
    "An ECSView can be used to iterate over all the entities containing a set of components",
    "[ECS]" )
{
    ECS ecs(1000);
    for (int i = 0; i < 10; i++)
    {
        EntityId entity = ecs.newEntity();
        int& entityInt = ecs.assign<int>(entity);
        entityInt = i * 2;
        if (i % 3 == 0)
        {
            std::string& entityString = ecs.assign<std::string>(entity);
            entityString = "Entity " + std::to_string(i);
        }
        if (i > 4)
        {
            Position& entityPosition = ecs.assign<Position>(entity);
            entityPosition = { 0, i , 0 };
        }
    }

    int i = 0;
    for (EntityId entity : ECSView<>(ecs))
    {
        REQUIRE( ecs.get<int>(entity) == i * 2 );
        i++;
    }
    auto itr = ECSView<std::string>(ecs).begin();
    for (int i = 0; i < 10; i += 3)
    {
        REQUIRE( ecs.get<std::string>(*itr) == "Entity " + std::to_string(i) );
        ++itr;
    }
    i = 0;
    for (EntityId entity : ECSView<std::string, Position>(ecs))
    {
        if (i == 0)
            REQUIRE( ecs.get<int>(entity) == 12 );
        else
            REQUIRE( ecs.get<int>(entity) == 18 );
        i = 1;
    }
    i = 0;
    for (EntityId entity : ECSView<Position, std::string>(ecs))
    {
        if (i == 0)
            REQUIRE( ecs.get<int>(entity) == 12 );
        else
            REQUIRE( ecs.get<int>(entity) == 18 );
        i = 1;
    }
}