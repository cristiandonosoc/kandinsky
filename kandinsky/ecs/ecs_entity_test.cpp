#include <catch2/catch_test_macros.hpp>

#include <kandinsky/ecs/ecs_entity.h>

using namespace kdk;

namespace kdk::ecs_entity_test_private {

void VerifyEntityComponentMatch(ECSEntityManager* eem,
                                ECSEntity entity,
                                EECSComponentType component_type,
                                bool should_match) {
    INFO("Entity: " << GetEntityIndex(entity) << ", COMPONENT: " << ToString(component_type));

    auto [ok, signature] = GetEntitySignature(eem, entity);
    REQUIRE(ok);
    if (should_match) {
        REQUIRE(Matches(*signature, component_type));
        ECSComponentIndex component_index = GetComponentIndex(*eem, entity, component_type);
        REQUIRE(component_index != NONE);
        REQUIRE(GetOwningEntity(*eem, component_type, component_index) == entity);
    } else {
        REQUIRE_FALSE(Matches(*signature, component_type));
        ECSComponentIndex component_index = GetComponentIndex(*eem, entity, component_type);
        REQUIRE(component_index == NONE);
    }
}

template <typename T>
void VerifyEntityComponentMatch(ECSEntityManager* eem, ECSEntity entity, bool should_match) {
    return VerifyEntityComponentMatch(eem, entity, T::kComponentType, should_match);
}

}  // namespace kdk::ecs_entity_test_private

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
        REQUIRE(AddComponent(eem, entity, EECSComponentType::EntityBase));

        {
            auto [ok, signature] = GetEntitySignature(eem, entity);

            i32 entity_index = GetEntityIndex(entity);

            REQUIRE(ok);
            REQUIRE(Matches(*signature, EECSComponentType::EntityBase));
            REQUIRE(eem->EntityBaseComponentHolder.ComponentCount == 1);
            REQUIRE(eem->EntityBaseComponentHolder.EntityToComponent[entity_index] == 0);
            REQUIRE(eem->EntityBaseComponentHolder.ComponentToEntity[0] == entity);
        }
    }

    SECTION("Add component to invalid entity") {
        CREATE_NEW_EEM(eem);

        // Try to add component to NONE entity
        REQUIRE_FALSE(AddComponent(eem, NONE, EECSComponentType::EntityBase));

        // Try to add component to destroyed entity
        ECSEntity entity = CreateEntity(eem);
        DestroyEntity(eem, entity);
        REQUIRE_FALSE(AddComponent(eem, entity, EECSComponentType::EntityBase));

        // Try to add component to entity with wrong generation
        ECSEntity new_entity = CreateEntity(eem);  // Reuses the same slot
        ECSEntity wrong_gen_entity =
            BuildEntity(GetEntityIndex(new_entity), GetEntityGeneration(new_entity) - 1);
        REQUIRE_FALSE(AddComponent(eem, wrong_gen_entity, EECSComponentType::EntityBase));

        // The new entity should work.
        REQUIRE(AddComponent(eem, new_entity, EECSComponentType::EntityBase));
    }

    SECTION("Add same component multiple times") {
        CREATE_NEW_EEM(eem);

        ECSEntity entity = CreateEntity(eem);

        // First addition should succeed
        REQUIRE(AddComponent(eem, entity, EECSComponentType::EntityBase));

        // Second addition should fail
        REQUIRE_FALSE(AddComponent(eem, entity, EECSComponentType::EntityBase));

        // Component count should still be 1
        REQUIRE(eem->EntityBaseComponentHolder.ComponentCount == 1);
    }

    SECTION("Add component after entity destruction") {
        CREATE_NEW_EEM(eem);

        ECSEntity entity = CreateEntity(eem);
        REQUIRE(AddComponent(eem, entity, EECSComponentType::EntityBase));

        DestroyEntity(eem, entity);

        // Component should be removed when entity is destroyed
        REQUIRE(eem->EntityBaseComponentHolder.ComponentCount == 0);
        REQUIRE(eem->EntityBaseComponentHolder.EntityToComponent[GetEntityIndex(entity)] == NONE);

        // Adding component to destroyed entity should fail
        REQUIRE_FALSE(AddComponent(eem, entity, EECSComponentType::EntityBase));
    }

    SECTION("Add components to multiple entities") {
        CREATE_NEW_EEM(eem);

        std::array<ECSEntity, 3> entities;
        for (auto& entity : entities) {
            entity = CreateEntity(eem);
            REQUIRE(AddComponent(eem, entity, EECSComponentType::EntityBase));
        }

        // Verify all entities have components
        for (const auto& entity : entities) {
            auto [ok, signature] = GetEntitySignature(eem, entity);
            REQUIRE(ok);
            REQUIRE(Matches(*signature, EECSComponentType::EntityBase));
            REQUIRE(eem->EntityBaseComponentHolder.EntityToComponent[GetEntityIndex(entity)] !=
                    NONE);
        }

        REQUIRE(eem->EntityBaseComponentHolder.ComponentCount == 3);
    }
}

TEST_CASE("ECS Component Removal", "[ecs]") {
    SECTION("Remove component from valid entity") {
        CREATE_NEW_EEM(eem);

        ECSEntity entity = CreateEntity(eem);
        REQUIRE(AddComponent(eem, entity, EECSComponentType::EntityBase));
        REQUIRE(RemoveComponent(eem, entity, EECSComponentType::EntityBase));

        {
            auto [ok, signature] = GetEntitySignature(eem, entity);
            i32 entity_index = GetEntityIndex(entity);

            REQUIRE(ok);
            REQUIRE_FALSE(Matches(*signature, EECSComponentType::EntityBase));
            REQUIRE(eem->EntityBaseComponentHolder.ComponentCount == 0);
            REQUIRE(eem->EntityBaseComponentHolder.EntityToComponent[entity_index] == NONE);
        }
    }

    SECTION("Remove component from invalid entity") {
        CREATE_NEW_EEM(eem);

        // Try to remove component from NONE entity
        REQUIRE_FALSE(RemoveComponent(eem, NONE, EECSComponentType::EntityBase));

        // Try to remove component from destroyed entity
        ECSEntity entity = CreateEntity(eem);
        REQUIRE(AddComponent(eem, entity, EECSComponentType::EntityBase));
        DestroyEntity(eem, entity);
        REQUIRE_FALSE(RemoveComponent(eem, entity, EECSComponentType::EntityBase));

        // Try to remove component from entity with wrong generation
        ECSEntity new_entity = CreateEntity(eem);  // Reuses the same slot
        REQUIRE(AddComponent(eem, new_entity, EECSComponentType::EntityBase));
        ECSEntity wrong_gen_entity =
            BuildEntity(GetEntityIndex(new_entity), GetEntityGeneration(new_entity) - 1);
        REQUIRE_FALSE(RemoveComponent(eem, wrong_gen_entity, EECSComponentType::EntityBase));

        // The new entity should work
        REQUIRE(RemoveComponent(eem, new_entity, EECSComponentType::EntityBase));
    }

    SECTION("Remove non-existent component") {
        CREATE_NEW_EEM(eem);

        ECSEntity entity = CreateEntity(eem);

        // Try to remove component that was never added
        REQUIRE_FALSE(RemoveComponent(eem, entity, EECSComponentType::EntityBase));

        // Add and remove the component
        REQUIRE(AddComponent(eem, entity, EECSComponentType::EntityBase));
        REQUIRE(RemoveComponent(eem, entity, EECSComponentType::EntityBase));

        // Try to remove it again
        REQUIRE_FALSE(RemoveComponent(eem, entity, EECSComponentType::EntityBase));
    }

    SECTION("Remove components from multiple entities") {
        using namespace kdk::ecs_entity_test_private;

        CREATE_NEW_EEM(eem);

        std::array<ECSEntity, 3> entities;
        for (auto& entity : entities) {
            entity = CreateEntity(eem);
            REQUIRE(AddComponent(eem, entity, EECSComponentType::EntityBase));
        }

        // Remove component from middle entity
        REQUIRE(RemoveComponent(eem, entities[1], EECSComponentType::EntityBase));
        REQUIRE(eem->EntityBaseComponentHolder.ComponentCount == 2);

        VerifyEntityComponentMatch(eem, entities[0], EntityBaseComponent::kComponentType, true);
        REQUIRE(eem->EntityBaseComponentHolder.EntityToComponent[GetEntityIndex(entities[1])] ==
                NONE);
        VerifyEntityComponentMatch(eem, entities[2], EntityBaseComponent::kComponentType, true);

        // Remove remaining components
        REQUIRE(RemoveComponent(eem, entities[0], EECSComponentType::EntityBase));
        REQUIRE(eem->EntityBaseComponentHolder.ComponentCount == 1);
        REQUIRE(eem->EntityBaseComponentHolder.EntityToComponent[GetEntityIndex(entities[0])] ==
                NONE);
        REQUIRE(eem->EntityBaseComponentHolder.EntityToComponent[GetEntityIndex(entities[1])] ==
                NONE);
        VerifyEntityComponentMatch(eem, entities[2], EntityBaseComponent::kComponentType, true);

        REQUIRE(RemoveComponent(eem, entities[2], EECSComponentType::EntityBase));
        REQUIRE(eem->EntityBaseComponentHolder.ComponentCount == 0);
        REQUIRE(eem->EntityBaseComponentHolder.EntityToComponent[GetEntityIndex(entities[0])] ==
                NONE);
        REQUIRE(eem->EntityBaseComponentHolder.EntityToComponent[GetEntityIndex(entities[1])] ==
                NONE);
        REQUIRE(eem->EntityBaseComponentHolder.EntityToComponent[GetEntityIndex(entities[2])] ==
                NONE);
    }
}

TEST_CASE("ECS Multiple Component Management", "[ecs]") {
    SECTION("Add and remove multiple components") {
        using namespace kdk::ecs_entity_test_private;

        CREATE_NEW_EEM(eem);

        ECSEntity entity = CreateEntity(eem);

        // Add both components
        REQUIRE(AddComponent(eem, entity, EECSComponentType::EntityBase));
        REQUIRE(AddComponent(eem, entity, EECSComponentType::Test));

        {
            INFO("Verify both components are present");
            auto [ok, signature] = GetEntitySignature(eem, entity);
            REQUIRE(ok);
            REQUIRE(GetComponentCount<EntityBaseComponent>(*eem) == 1);
            REQUIRE(GetComponentCount<TestComponent>(*eem) == 1);
            VerifyEntityComponentMatch<EntityBaseComponent>(eem, entity, true);
            VerifyEntityComponentMatch<TestComponent>(eem, entity, true);
        }

        {
            INFO("Remove one component");
            REQUIRE(RemoveComponent(eem, entity, EECSComponentType::EntityBase));

            auto [ok, signature] = GetEntitySignature(eem, entity);
            REQUIRE(ok);
            REQUIRE(GetComponentCount<EntityBaseComponent>(*eem) == 0);
            REQUIRE(GetComponentCount<TestComponent>(*eem) == 1);
            VerifyEntityComponentMatch<EntityBaseComponent>(eem, entity, false);
            VerifyEntityComponentMatch<TestComponent>(eem, entity, true);
        }

        {
            INFO("Verify all components are removed");
            REQUIRE(RemoveComponent(eem, entity, EECSComponentType::Test));
            auto [ok, signature] = GetEntitySignature(eem, entity);

            REQUIRE(ok);
            REQUIRE(GetComponentCount<EntityBaseComponent>(*eem) == 0);
            REQUIRE(GetComponentCount<TestComponent>(*eem) == 0);
            VerifyEntityComponentMatch<EntityBaseComponent>(eem, entity, false);
            VerifyEntityComponentMatch<TestComponent>(eem, entity, false);
        }
    }

    SECTION("Multiple entities with different component combinations") {
        using namespace kdk::ecs_entity_test_private;

        CREATE_NEW_EEM(eem);

        ECSEntity e1 = CreateEntity(eem);  // Will have both components
        ECSEntity e2 = CreateEntity(eem);  // Will have only EntityBase
        ECSEntity e3 = CreateEntity(eem);  // Will have only Test

        {
            INFO("Add components in different combinations");
            REQUIRE(AddComponent(eem, e1, EECSComponentType::EntityBase));
            REQUIRE(AddComponent(eem, e1, EECSComponentType::Test));
            REQUIRE(AddComponent(eem, e2, EECSComponentType::EntityBase));
            REQUIRE(AddComponent(eem, e3, EECSComponentType::Test));

            REQUIRE(GetComponentCount<EntityBaseComponent>(*eem) == 2);
            REQUIRE(GetComponentCount<TestComponent>(*eem) == 2);

            VerifyEntityComponentMatch<EntityBaseComponent>(eem, e1, true);
            VerifyEntityComponentMatch<TestComponent>(eem, e1, true);
            VerifyEntityComponentMatch<EntityBaseComponent>(eem, e2, true);
            VerifyEntityComponentMatch<TestComponent>(eem, e2, false);
            VerifyEntityComponentMatch<EntityBaseComponent>(eem, e3, false);
            VerifyEntityComponentMatch<TestComponent>(eem, e3, true);
        }

        {
            INFO("Remove components in mixed order");
            REQUIRE(RemoveComponent(eem, e1, EECSComponentType::Test));
            REQUIRE(RemoveComponent(eem, e2, EECSComponentType::EntityBase));
            REQUIRE(RemoveComponent(eem, e3, EECSComponentType::Test));

            REQUIRE(GetComponentCount<EntityBaseComponent>(*eem) == 1);
            REQUIRE(GetComponentCount<TestComponent>(*eem) == 0);

            VerifyEntityComponentMatch<EntityBaseComponent>(eem, e1, true);
            VerifyEntityComponentMatch<TestComponent>(eem, e1, false);
            VerifyEntityComponentMatch<EntityBaseComponent>(eem, e2, false);
            VerifyEntityComponentMatch<TestComponent>(eem, e2, false);
            VerifyEntityComponentMatch<EntityBaseComponent>(eem, e3, false);
            VerifyEntityComponentMatch<TestComponent>(eem, e3, false);
        }
    }

    SECTION("Entity destruction with multiple components") {
        using namespace kdk::ecs_entity_test_private;

        CREATE_NEW_EEM(eem);

        ECSEntity entity = CreateEntity(eem);

        // Add both components
        REQUIRE(AddComponent(eem, entity, EECSComponentType::EntityBase));
        REQUIRE(AddComponent(eem, entity, EECSComponentType::Test));
        REQUIRE(GetComponentCount<EntityBaseComponent>(*eem) == 1);
        REQUIRE(GetComponentCount<TestComponent>(*eem) == 1);

        {
            INFO("Verify initial state");
            VerifyEntityComponentMatch<EntityBaseComponent>(eem, entity, true);
            VerifyEntityComponentMatch<TestComponent>(eem, entity, true);
        }

        {
            INFO("Destroy entity");
            DestroyEntity(eem, entity);

            REQUIRE(GetComponentCount<EntityBaseComponent>(*eem) == 0);
            REQUIRE(GetComponentCount<TestComponent>(*eem) == 0);
        }
    }
}
