#include <catch2/catch_test_macros.hpp>

#include <kandinsky/ecs/ecs_entity.h>

using namespace kdk;

#define CREATE_NEW_EEM(var_name) \
    ECSEntityManager var_name;   \
    Init(&var_name);             \
    DEFER { Shutdown(&var_name); }

TEST_CASE("ECS Entity Creation and Destruction", "[ecs]") {
    SECTION("Initial state is correct") {
        CREATE_NEW_EEM(eem);
        REQUIRE(eem.EntityCount == 0);
        REQUIRE(eem.NextEntity == 0);
    }

    SECTION("Create single entity") {
        CREATE_NEW_EEM(eem);

        ECSEntity entity = CreateEntity(&eem);
        REQUIRE(entity == 0);
        REQUIRE(eem.EntityCount == 1);
        REQUIRE(eem.NextEntity == 1);
        REQUIRE(eem.Signatures[entity] == 0);
    }

    SECTION("Create multiple entities") {
        CREATE_NEW_EEM(eem);

        ECSEntity e1 = CreateEntity(&eem);
        REQUIRE(eem.NextEntity == 1);
        ECSEntity e2 = CreateEntity(&eem);
        REQUIRE(eem.NextEntity == 2);
        ECSEntity e3 = CreateEntity(&eem);
        REQUIRE(eem.NextEntity == 3);

        REQUIRE(e1 == 0);
        REQUIRE(e2 == 1);
        REQUIRE(e3 == 2);
        REQUIRE(eem.EntityCount == 3);
    }

    SECTION("Destroy entity and create new one") {
        CREATE_NEW_EEM(eem);

        ECSEntity e1 = CreateEntity(&eem);
        DestroyEntity(&eem, e1);
        REQUIRE(eem.EntityCount == 0);
        REQUIRE(eem.NextEntity == 0);

        // Creating a new entity should reuse the destroyed slot
        ECSEntity e2 = CreateEntity(&eem);
        REQUIRE(e2 == e1);
        REQUIRE(eem.EntityCount == 1);
        REQUIRE(eem.NextEntity == 1);
    }

    SECTION("Create and destroy multiple entities") {
        CREATE_NEW_EEM(eem);

        std::array<ECSEntity, 5> entities;
        for (u32 i = 0; i < entities.size(); ++i) {
            entities[i] = CreateEntity(&eem);
        }
        REQUIRE(eem.EntityCount == 5);
        REQUIRE(eem.NextEntity == 5);
        REQUIRE(eem.Signatures[0] == 0);
        REQUIRE(eem.Signatures[1] == 0);
        REQUIRE(eem.Signatures[2] == 0);
        REQUIRE(eem.Signatures[3] == 0);
        REQUIRE(eem.Signatures[4] == 0);
        REQUIRE(eem.Signatures[5] == 6);

        DestroyEntity(&eem, entities[1]);  // Destroy middle entity
        REQUIRE(eem.EntityCount == 4);
        REQUIRE(eem.NextEntity == 1);
        REQUIRE(eem.Signatures[0] == 0);
        REQUIRE(eem.Signatures[1] == 5);
        REQUIRE(eem.Signatures[2] == 0);
        REQUIRE(eem.Signatures[3] == 0);
        REQUIRE(eem.Signatures[4] == 0);
        REQUIRE(eem.Signatures[5] == 6);

        DestroyEntity(&eem, entities[4]);  // Destroy another middle entity
        REQUIRE(eem.EntityCount == 3);
        REQUIRE(eem.NextEntity == 4);
        REQUIRE(eem.Signatures[0] == 0);
        REQUIRE(eem.Signatures[1] == 5);
        REQUIRE(eem.Signatures[2] == 0);
        REQUIRE(eem.Signatures[3] == 0);
        REQUIRE(eem.Signatures[4] == 1);
        REQUIRE(eem.Signatures[5] == 6);

		ECSEntity newEntity = NONE;

        // Next created entity should use the freed slot
        newEntity = CreateEntity(&eem);
        REQUIRE(newEntity == 4);
        REQUIRE(eem.EntityCount == 4);
        REQUIRE(eem.NextEntity == 1);
        REQUIRE(eem.Signatures[0] == 0);
        REQUIRE(eem.Signatures[1] == 5);
        REQUIRE(eem.Signatures[2] == 0);
        REQUIRE(eem.Signatures[3] == 0);
        REQUIRE(eem.Signatures[4] == 0);
        REQUIRE(eem.Signatures[5] == 6);

        // Next created entity should use the freed slot
        newEntity = CreateEntity(&eem);
        REQUIRE(newEntity == 1);
        REQUIRE(eem.EntityCount == 5);
        REQUIRE(eem.NextEntity == 5);
        REQUIRE(eem.Signatures[0] == 0);
        REQUIRE(eem.Signatures[1] == 0);
        REQUIRE(eem.Signatures[2] == 0);
        REQUIRE(eem.Signatures[3] == 0);
        REQUIRE(eem.Signatures[4] == 0);
        REQUIRE(eem.Signatures[5] == 6);
    }
}
