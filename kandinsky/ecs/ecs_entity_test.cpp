#include <catch2/catch_test_macros.hpp>

#include <kandinsky/ecs/ecs_entity.h>

using namespace kdk;

#define CREATE_NEW_EEM(var_name)                         \
    ECSEntityManager* var_name = new ECSEntityManager(); \
    Init(var_name);                                      \
    DEFER {                                              \
        Shutdown(var_name);                              \
        delete var_name;                                 \
    }

TEST_CASE("ECS Entity Creation and Destruction", "[ecs]") {
    SECTION("Initial state is correct") {
        CREATE_NEW_EEM(eem);
        REQUIRE(eem->EntityCount == 0);
        REQUIRE(eem->NextIndex == 0);
    }

    SECTION("Create single entity") {
        CREATE_NEW_EEM(eem);

        ECSEntity entity = CreateEntity(eem);
        REQUIRE(GetEntityIndex(entity) == 0);
        REQUIRE(GetEntityGeneration(entity) == 1);
        REQUIRE(eem->EntityCount == 1);
        REQUIRE(eem->NextIndex == 1);
        REQUIRE(eem->Signatures[GetEntityIndex(entity)] == kNewEntitySignature);
    }

    SECTION("Create multiple entities") {
        CREATE_NEW_EEM(eem);

        ECSEntity e1 = CreateEntity(eem);
        REQUIRE(eem->NextIndex == 1);
        ECSEntity e2 = CreateEntity(eem);
        REQUIRE(eem->NextIndex == 2);
        ECSEntity e3 = CreateEntity(eem);
        REQUIRE(eem->NextIndex == 3);

        REQUIRE(eem->EntityCount == 3);
        REQUIRE(GetEntityIndex(e1) == 0);
        REQUIRE(GetEntityGeneration(e1) == 1);
        REQUIRE(GetEntityIndex(e2) == 1);
        REQUIRE(GetEntityGeneration(e2) == 1);
        REQUIRE(GetEntityIndex(e3) == 2);
        REQUIRE(GetEntityGeneration(e3) == 1);
    }

    SECTION("Destroy entity and create new one") {
        CREATE_NEW_EEM(eem);

        ECSEntity e1 = CreateEntity(eem);
        REQUIRE(eem->EntityCount == 1);
        REQUIRE(eem->NextIndex == 1);
        REQUIRE(GetEntityIndex(e1) == 0);
        REQUIRE(GetEntityGeneration(e1) == 1);

        DestroyEntity(eem, e1);
        REQUIRE(eem->EntityCount == 0);
        REQUIRE(eem->NextIndex == 0);
        REQUIRE(GetEntityIndex(e1) == 0);
        REQUIRE(GetEntityGeneration(e1) == 1);

        // Creating a new entity should reuse the destroyed slot
        ECSEntity e2 = CreateEntity(eem);
        REQUIRE(eem->EntityCount == 1);
        REQUIRE(eem->NextIndex == 1);
        REQUIRE(GetEntityIndex(e2) == 0);
        REQUIRE(GetEntityGeneration(e2) == 2);
    }

    SECTION("Create and destroy multiple entities") {
        CREATE_NEW_EEM(eem);

        std::array<ECSEntity, 5> entities;
        for (u32 i = 0; i < entities.size(); ++i) {
            entities[i] = CreateEntity(eem);
        }
        REQUIRE(eem->EntityCount == 5);
        REQUIRE(eem->NextIndex == 5);
        REQUIRE(eem->Signatures[0] == kNewEntitySignature);
        REQUIRE(eem->Signatures[1] == kNewEntitySignature);
        REQUIRE(eem->Signatures[2] == kNewEntitySignature);
        REQUIRE(eem->Signatures[3] == kNewEntitySignature);
        REQUIRE(eem->Signatures[4] == kNewEntitySignature);
        REQUIRE(eem->Signatures[5] == 6);
        REQUIRE(eem->Generations[0] == 1);
        REQUIRE(eem->Generations[1] == 1);
        REQUIRE(eem->Generations[2] == 1);
        REQUIRE(eem->Generations[3] == 1);
        REQUIRE(eem->Generations[4] == 1);
        REQUIRE(eem->Generations[5] == 0);

        DestroyEntity(eem, entities[1]);  // Destroy middle entity
        REQUIRE(eem->EntityCount == 4);
        REQUIRE(eem->NextIndex == 1);
        REQUIRE(eem->Signatures[0] == kNewEntitySignature);
        REQUIRE(eem->Signatures[1] == 5);
        REQUIRE(eem->Signatures[2] == kNewEntitySignature);
        REQUIRE(eem->Signatures[3] == kNewEntitySignature);
        REQUIRE(eem->Signatures[4] == kNewEntitySignature);
        REQUIRE(eem->Signatures[5] == 6);
        REQUIRE(eem->Generations[0] == 1);
        REQUIRE(eem->Generations[1] == 1);
        REQUIRE(eem->Generations[2] == 1);
        REQUIRE(eem->Generations[3] == 1);
        REQUIRE(eem->Generations[4] == 1);
        REQUIRE(eem->Generations[5] == 0);

        DestroyEntity(eem, entities[4]);  // Destroy another middle entity
        REQUIRE(eem->EntityCount == 3);
        REQUIRE(eem->NextIndex == 4);
        REQUIRE(eem->Signatures[0] == kNewEntitySignature);
        REQUIRE(eem->Signatures[1] == 5);
        REQUIRE(eem->Signatures[2] == kNewEntitySignature);
        REQUIRE(eem->Signatures[3] == kNewEntitySignature);
        REQUIRE(eem->Signatures[4] == 1);
        REQUIRE(eem->Signatures[5] == 6);
        REQUIRE(eem->Generations[0] == 1);
        REQUIRE(eem->Generations[1] == 1);
        REQUIRE(eem->Generations[2] == 1);
        REQUIRE(eem->Generations[3] == 1);
        REQUIRE(eem->Generations[4] == 1);
        REQUIRE(eem->Generations[5] == 0);

        ECSEntity newEntity = NONE;

        // Next created entity should use the freed slot
        newEntity = CreateEntity(eem);
        REQUIRE(GetEntityIndex(newEntity) == 4);
        REQUIRE(eem->EntityCount == 4);
        REQUIRE(eem->NextIndex == 1);
        REQUIRE(eem->Signatures[0] == kNewEntitySignature);
        REQUIRE(eem->Signatures[1] == 5);
        REQUIRE(eem->Signatures[2] == kNewEntitySignature);
        REQUIRE(eem->Signatures[3] == kNewEntitySignature);
        REQUIRE(eem->Signatures[4] == kNewEntitySignature);
        REQUIRE(eem->Signatures[5] == 6);
        REQUIRE(eem->Generations[0] == 1);
        REQUIRE(eem->Generations[1] == 1);
        REQUIRE(eem->Generations[2] == 1);
        REQUIRE(eem->Generations[3] == 1);
        REQUIRE(eem->Generations[4] == 2);
        REQUIRE(eem->Generations[5] == 0);

        // Next created entity should use the freed slot
        newEntity = CreateEntity(eem);
        REQUIRE(GetEntityIndex(newEntity) == 1);
        REQUIRE(eem->EntityCount == 5);
        REQUIRE(eem->NextIndex == 5);
        REQUIRE(eem->Signatures[0] == kNewEntitySignature);
        REQUIRE(eem->Signatures[1] == kNewEntitySignature);
        REQUIRE(eem->Signatures[2] == kNewEntitySignature);
        REQUIRE(eem->Signatures[3] == kNewEntitySignature);
        REQUIRE(eem->Signatures[4] == kNewEntitySignature);
        REQUIRE(eem->Signatures[5] == 6);
        REQUIRE(eem->Generations[0] == 1);
        REQUIRE(eem->Generations[1] == 2);
        REQUIRE(eem->Generations[2] == 1);
        REQUIRE(eem->Generations[3] == 1);
        REQUIRE(eem->Generations[4] == 2);
        REQUIRE(eem->Generations[5] == 0);

        // Next created entity should use the go forward.
        newEntity = CreateEntity(eem);
        REQUIRE(GetEntityIndex(newEntity) == 5);
        REQUIRE(eem->EntityCount == 6);
        REQUIRE(eem->NextIndex == 6);
        REQUIRE(eem->Signatures[0] == kNewEntitySignature);
        REQUIRE(eem->Signatures[1] == kNewEntitySignature);
        REQUIRE(eem->Signatures[2] == kNewEntitySignature);
        REQUIRE(eem->Signatures[3] == kNewEntitySignature);
        REQUIRE(eem->Signatures[4] == kNewEntitySignature);
        REQUIRE(eem->Signatures[5] == kNewEntitySignature);
        REQUIRE(eem->Signatures[6] == 7);
        REQUIRE(eem->Generations[0] == 1);
        REQUIRE(eem->Generations[1] == 2);
        REQUIRE(eem->Generations[2] == 1);
        REQUIRE(eem->Generations[3] == 1);
        REQUIRE(eem->Generations[4] == 2);
        REQUIRE(eem->Generations[5] == 1);
        REQUIRE(eem->Generations[6] == 0);
    }
}

TEST_CASE("ECS Component Addition", "[ecs]") {
    SECTION("Add component to valid entity") {
        CREATE_NEW_EEM(eem);

        ECSEntity entity = CreateEntity(eem);
        REQUIRE(AddComponent(eem, entity, EECSComponentType::Transform));

        {
            auto [ok, signature] = GetEntitySignature(eem, entity);

            i32 entity_index = GetEntityIndex(entity);

            REQUIRE(ok);
            REQUIRE(Matches(*signature, EECSComponentType::Transform));
            REQUIRE(eem->TransformHolder.ComponentCount == 1);
            REQUIRE(eem->TransformHolder.EntityToComponent[entity_index] == 0);
            REQUIRE(eem->TransformHolder.ComponentToEntity[0] == entity);
        }
    }

    SECTION("Add component to invalid entity") {
        CREATE_NEW_EEM(eem);

        // Try to add component to NONE entity
        REQUIRE_FALSE(AddComponent(eem, NONE, EECSComponentType::Transform));

        // Try to add component to destroyed entity
        ECSEntity entity = CreateEntity(eem);
        DestroyEntity(eem, entity);
        REQUIRE_FALSE(AddComponent(eem, entity, EECSComponentType::Transform));

        // Try to add component to entity with wrong generation
        ECSEntity new_entity = CreateEntity(eem);  // Reuses the same slot
        ECSEntity wrong_gen_entity =
            BuildEntity(GetEntityIndex(new_entity), GetEntityGeneration(new_entity) - 1);
        REQUIRE_FALSE(AddComponent(eem, wrong_gen_entity, EECSComponentType::Transform));

        // The new entity should work.
        REQUIRE(AddComponent(eem, new_entity, EECSComponentType::Transform));
    }

    SECTION("Add same component multiple times") {
        CREATE_NEW_EEM(eem);

        ECSEntity entity = CreateEntity(eem);

        // First addition should succeed
        REQUIRE(AddComponent(eem, entity, EECSComponentType::Transform));

        // Second addition should fail
        REQUIRE_FALSE(AddComponent(eem, entity, EECSComponentType::Transform));

        // Component count should still be 1
        REQUIRE(eem->TransformHolder.ComponentCount == 1);
    }

    SECTION("Add component after entity destruction") {
        CREATE_NEW_EEM(eem);

        ECSEntity entity = CreateEntity(eem);
        REQUIRE(AddComponent(eem, entity, EECSComponentType::Transform));

        DestroyEntity(eem, entity);

        // Component should be removed when entity is destroyed
        REQUIRE(eem->TransformHolder.ComponentCount == 0);
        REQUIRE(eem->TransformHolder.EntityToComponent[GetEntityIndex(entity)] == NONE);

        // Adding component to destroyed entity should fail
        REQUIRE_FALSE(AddComponent(eem, entity, EECSComponentType::Transform));
    }

    SECTION("Add components to multiple entities") {
        CREATE_NEW_EEM(eem);

        std::array<ECSEntity, 3> entities;
        for (auto& entity : entities) {
            entity = CreateEntity(eem);
            REQUIRE(AddComponent(eem, entity, EECSComponentType::Transform));
        }

        // Verify all entities have components
        for (const auto& entity : entities) {
            auto [ok, signature] = GetEntitySignature(eem, entity);
            REQUIRE(ok);
            REQUIRE(Matches(*signature, EECSComponentType::Transform));
            REQUIRE(eem->TransformHolder.EntityToComponent[GetEntityIndex(entity)] != NONE);
        }

        REQUIRE(eem->TransformHolder.ComponentCount == 3);
    }
}

TEST_CASE("ECS Component Removal", "[ecs]") {
    SECTION("Remove component from valid entity") {
        CREATE_NEW_EEM(eem);

        ECSEntity entity = CreateEntity(eem);
        REQUIRE(AddComponent(eem, entity, EECSComponentType::Transform));
        REQUIRE(RemoveComponent(eem, entity, EECSComponentType::Transform));

        {
            auto [ok, signature] = GetEntitySignature(eem, entity);
            i32 entity_index = GetEntityIndex(entity);

            REQUIRE(ok);
            REQUIRE_FALSE(Matches(*signature, EECSComponentType::Transform));
            REQUIRE(eem->TransformHolder.ComponentCount == 0);
            REQUIRE(eem->TransformHolder.EntityToComponent[entity_index] == NONE);
        }
    }

    SECTION("Remove component from invalid entity") {
        CREATE_NEW_EEM(eem);

        // Try to remove component from NONE entity
        REQUIRE_FALSE(RemoveComponent(eem, NONE, EECSComponentType::Transform));

        // Try to remove component from destroyed entity
        ECSEntity entity = CreateEntity(eem);
        REQUIRE(AddComponent(eem, entity, EECSComponentType::Transform));
        DestroyEntity(eem, entity);
        REQUIRE_FALSE(RemoveComponent(eem, entity, EECSComponentType::Transform));

        // Try to remove component from entity with wrong generation
        ECSEntity new_entity = CreateEntity(eem);  // Reuses the same slot
        REQUIRE(AddComponent(eem, new_entity, EECSComponentType::Transform));
        ECSEntity wrong_gen_entity =
            BuildEntity(GetEntityIndex(new_entity), GetEntityGeneration(new_entity) - 1);
        REQUIRE_FALSE(RemoveComponent(eem, wrong_gen_entity, EECSComponentType::Transform));

        // The new entity should work
        REQUIRE(RemoveComponent(eem, new_entity, EECSComponentType::Transform));
    }

    SECTION("Remove non-existent component") {
        CREATE_NEW_EEM(eem);

        ECSEntity entity = CreateEntity(eem);

        // Try to remove component that was never added
        REQUIRE_FALSE(RemoveComponent(eem, entity, EECSComponentType::Transform));

        // Add and remove the component
        REQUIRE(AddComponent(eem, entity, EECSComponentType::Transform));
        REQUIRE(RemoveComponent(eem, entity, EECSComponentType::Transform));

        // Try to remove it again
        REQUIRE_FALSE(RemoveComponent(eem, entity, EECSComponentType::Transform));
    }

    SECTION("Remove components from multiple entities") {
        CREATE_NEW_EEM(eem);

        std::array<ECSEntity, 3> entities;
        for (auto& entity : entities) {
            entity = CreateEntity(eem);
            REQUIRE(AddComponent(eem, entity, EECSComponentType::Transform));
        }

        auto verify_match = [eem](ECSEntity entity) {
            auto [ok, signature] = GetEntitySignature(eem, entity);
            i32 entity_index = GetEntityIndex(entity);
            REQUIRE(ok);
            REQUIRE(Matches(*signature, EECSComponentType::Transform));
            ECSComponentIndex component_index =
                eem->TransformHolder.EntityToComponent[entity_index];
            REQUIRE(component_index != NONE);
            REQUIRE(eem->TransformHolder.ComponentToEntity[component_index] == entity);
        };

        // Remove component from middle entity
        REQUIRE(RemoveComponent(eem, entities[1], EECSComponentType::Transform));
        REQUIRE(eem->TransformHolder.ComponentCount == 2);

        verify_match(entities[0]);
        REQUIRE(eem->TransformHolder.EntityToComponent[GetEntityIndex(entities[1])] == NONE);
        verify_match(entities[2]);

        // Remove remaining components
        REQUIRE(RemoveComponent(eem, entities[0], EECSComponentType::Transform));
        REQUIRE(eem->TransformHolder.ComponentCount == 1);
        REQUIRE(eem->TransformHolder.EntityToComponent[GetEntityIndex(entities[0])] == NONE);
        REQUIRE(eem->TransformHolder.EntityToComponent[GetEntityIndex(entities[1])] == NONE);
        verify_match(entities[2]);

        REQUIRE(RemoveComponent(eem, entities[2], EECSComponentType::Transform));
        REQUIRE(eem->TransformHolder.ComponentCount == 0);
        REQUIRE(eem->TransformHolder.EntityToComponent[GetEntityIndex(entities[0])] == NONE);
        REQUIRE(eem->TransformHolder.EntityToComponent[GetEntityIndex(entities[1])] == NONE);
        REQUIRE(eem->TransformHolder.EntityToComponent[GetEntityIndex(entities[2])] == NONE);
    }
}
