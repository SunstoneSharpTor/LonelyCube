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

#include "core/entities/ECS.h"
#include "core/entities/ECSView.h"
#include "core/utils/iVec3.h"
#include <catch2/catch_test_macros.hpp>

using namespace lonelycube;

TEST_CASE( "Entities can be added, destroyed and have components assigned to them", "[ECS]" ) {
    ECS ecs(1000);
    EntityId entity1 = ecs.newEntity();
    IVec3& entity1Position = ecs.assign<IVec3>(entity1);
    entity1Position = { 0, 1, 0 };
    EntityId entity2 = ecs.newEntity();
    std::string& entity2String = ecs.assign<std::string>(entity2);
    entity2String = "Entity 2";
    std::string& entity1String = ecs.assign<std::string>(entity1);
    entity1String = "Entity 1";
    int& entity1Int = ecs.assign<int>(entity1);
    entity1Int = 1;
    IVec3& entity2Position = ecs.assign<IVec3>(entity2);
    entity2Position = { 0, 2, 0 };
    EntityId entity3 = ecs.newEntity();
    int& entity3Int = ecs.assign<int>(entity3);
    entity3Int = 3;

    SECTION( "Entities can be added" )
    {
        REQUIRE( ecs.get<IVec3>(entity1) == IVec3(0, 1, 0) );
        REQUIRE( entity1Position == IVec3(0, 1, 0) );
        REQUIRE( ecs.get<IVec3>(entity2) == IVec3(0, 2, 0) );
        REQUIRE( entity2Position == IVec3(0, 2, 0) );
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
    SECTION( "Components can have values assigned to them using the set method" )
    {
        ecs.set<std::string>(entity2, "New value");
        ecs.set<IVec3>(entity1, IVec3(1, 2, 3));

        REQUIRE( ecs.get<std::string>(entity2) == "New value" );
        REQUIRE( ecs.get<IVec3>(entity1) == IVec3(1, 2, 3) );
    }
    SECTION( "It is possible to check whether or not an entity has a given component" )
    {
        REQUIRE( ecs.entityHasComponent<IVec3>(entity1) == true );
        REQUIRE( ecs.entityHasComponent<std::string>(entity1) == true );
        REQUIRE( ecs.entityHasComponent<int>(entity1) == true );
        REQUIRE( ecs.entityHasComponent<IVec3>(entity2) == true );
        REQUIRE( ecs.entityHasComponent<std::string>(entity2) == true );
        REQUIRE( ecs.entityHasComponent<int>(entity2) == false );
        REQUIRE( ecs.entityHasComponent<IVec3>(entity3) == false );
        REQUIRE( ecs.entityHasComponent<std::string>(entity3) == false );
        REQUIRE( ecs.entityHasComponent<int>(entity3) == true );
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
            IVec3& entityPosition = ecs.assign<IVec3>(entity);
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
    for (EntityId entity : ECSView<std::string, IVec3>(ecs))
    {
        if (i == 0)
            REQUIRE( ecs.get<int>(entity) == 12 );
        else
            REQUIRE( ecs.get<int>(entity) == 18 );
        i = 1;
    }
    i = 0;
    for (EntityId entity : ECSView<IVec3, std::string>(ecs))
    {
        if (i == 0)
            REQUIRE( ecs.get<int>(entity) == 12 );
        else
            REQUIRE( ecs.get<int>(entity) == 18 );
        i = 1;
    }
}

TEST_CASE("Components can be constructed in place using any available constructor", "[ECS]" )
{
    struct Name
    {
        std::string name;

        Name() : name("Unnamed") {}
        Name(std::string name) : name(name) {}
        Name(std::string forename, std::string surname) : name(forename + " " + surname) {}
    };

    ECS ecs(1000);
    EntityId entity1 = ecs.newEntity();
    ecs.assign<Name>(entity1);
    EntityId entity2 = ecs.newEntity();
    ecs.assign<Name>(entity2, "Lonely Cube");
    EntityId entity3 = ecs.newEntity();
    ecs.assign<Name>(entity3, "Lonely", "Cube");

    REQUIRE( ecs.get<Name>(entity1).name == "Unnamed" );
    REQUIRE( ecs.get<Name>(entity2).name == "Lonely Cube" );
    REQUIRE( ecs.get<Name>(entity3).name == "Lonely Cube" );
}
