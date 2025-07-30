#include <catch2/catch_test_macros.hpp>

#include <kandinsky/entity.h>

using namespace kdk;

namespace kdk::ecs_entity_test_private {

void VerifyEntityComponentMatch(EntityManager* eem,
                                Entity entity,
                                EEntityComponentType component_type,
                                bool should_match) {
    INFO("Entity: " << GetEntityIndex(entity) << ", COMPONENT: " << ToString(component_type));

    auto* signature = GetEntitySignature(eem, entity);
    REQUIRE(signature);
    if (should_match) {
        REQUIRE(Matches(*signature, component_type));
        EntityComponentIndex component_index = GetComponentIndex(*eem, entity, component_type);
        REQUIRE(component_index != NONE);
        REQUIRE(GetOwningEntity(*eem, component_type, component_index) == entity);
    } else {
        REQUIRE_FALSE(Matches(*signature, component_type));
        EntityComponentIndex component_index = GetComponentIndex(*eem, entity, component_type);
        REQUIRE(component_index == NONE);
    }
}

template <typename T>
void VerifyEntityComponentMatch(EntityManager* eem, Entity entity, bool should_match) {
    return VerifyEntityComponentMatch(eem, entity, T::kComponentType, should_match);
}

}  // namespace kdk::ecs_entity_test_private

#define CREATE_NEW_EEM(var_name)                                    \
    Arena arena = AllocateArena(1 * MEGABYTE);                      \
    EntityManager* var_name = ArenaPushInit<EntityManager>(&arena); \
    Init(&arena, var_name);                                         \
    DEFER {                                                         \
        Shutdown(var_name);                                         \
        ArenaReset(&arena);                                         \
    }

TEST_CASE("ECS Entity Creation and Destruction: Initial state is correct", "[ecs]") {
    CREATE_NEW_EEM(eem);
    REQUIRE(eem->EntityCount == 0);
    REQUIRE(eem->NextIndex == 0);
}

TEST_CASE("ECS Entity Creation and Destruction: Create single entity", "[ecs]") {
    CREATE_NEW_EEM(eem);

    Entity entity = CreateEntity(eem);
    REQUIRE(GetEntityIndex(entity) == 0);
    REQUIRE(GetEntityGeneration(entity) == 1);
    REQUIRE(eem->EntityCount == 1);
    REQUIRE(eem->NextIndex == 1);
    REQUIRE(eem->Signatures[GetEntityIndex(entity)] == kNewEntitySignature);
}

TEST_CASE("ECS Entity Creation and Destruction: Create multiple entities", "[ecs]") {
    CREATE_NEW_EEM(eem);

    Entity e1 = CreateEntity(eem);
    REQUIRE(eem->NextIndex == 1);
    Entity e2 = CreateEntity(eem);
    REQUIRE(eem->NextIndex == 2);
    Entity e3 = CreateEntity(eem);
    REQUIRE(eem->NextIndex == 3);

    REQUIRE(eem->EntityCount == 3);
    REQUIRE(GetEntityIndex(e1) == 0);
    REQUIRE(GetEntityGeneration(e1) == 1);
    REQUIRE(GetEntityIndex(e2) == 1);
    REQUIRE(GetEntityGeneration(e2) == 1);
    REQUIRE(GetEntityIndex(e3) == 2);
    REQUIRE(GetEntityGeneration(e3) == 1);
}

TEST_CASE("ECS Entity Creation and Destruction: Destroy entity and create new one", "[ecs]") {
    CREATE_NEW_EEM(eem);

    Entity e1 = CreateEntity(eem);
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
    Entity e2 = CreateEntity(eem);
    REQUIRE(eem->EntityCount == 1);
    REQUIRE(eem->NextIndex == 1);
    REQUIRE(GetEntityIndex(e2) == 0);
    REQUIRE(GetEntityGeneration(e2) == 2);
}

TEST_CASE("ECS Entity Creation and Destruction: Create and destroy multiple entities", "[ecs]") {
    CREATE_NEW_EEM(eem);

    std::array<Entity, 5> entities;
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

    Entity newEntity = NONE;

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

TEST_CASE("Add component to valid entity", "[ecs]") {
    CREATE_NEW_EEM(eem);

    Entity entity = CreateEntity(eem);
    REQUIRE(AddComponent<Test2Component>(eem, entity));

    {
        auto* signature = GetEntitySignature(eem, entity);
        REQUIRE(signature);

        REQUIRE(Matches(*signature, EEntityComponentType::Test2));
        REQUIRE(GetComponentCount<Test2Component>(*eem) == 1);
        REQUIRE(GetComponentIndex<Test2Component>(*eem, entity) == 0);
        REQUIRE(GetOwningEntity<Test2Component>(*eem, 0) == entity);
    }
}

TEST_CASE("Add component to invalid entity", "[ecs]") {
    CREATE_NEW_EEM(eem);

    // Try to add component to NONE entity
    REQUIRE_FALSE(AddComponent(eem, NONE, EEntityComponentType::Test2));

    // Try to add component to destroyed entity
    Entity entity = CreateEntity(eem);
    DestroyEntity(eem, entity);
    REQUIRE_FALSE(AddComponent(eem, entity, EEntityComponentType::Test2));

    // Try to add component to entity with wrong generation
    Entity new_entity = CreateEntity(eem);  // Reuses the same slot
    Entity wrong_gen_entity =
        BuildEntity(GetEntityIndex(new_entity), GetEntityGeneration(new_entity) - 1);
    REQUIRE_FALSE(AddComponent(eem, wrong_gen_entity, EEntityComponentType::Test2));

    // The new entity should work.
    REQUIRE(AddComponent(eem, new_entity, EEntityComponentType::Test2));
}

TEST_CASE("Add same component multiple times", "[ecs]") {
    CREATE_NEW_EEM(eem);

    Entity entity = CreateEntity(eem);

    // First addition should succeed
    REQUIRE(AddComponent(eem, entity, EEntityComponentType::Test2));

    // Second addition should fail
    REQUIRE_FALSE(AddComponent(eem, entity, EEntityComponentType::Test2));

    // Component count should still be 1
    REQUIRE(GetComponentCount<Test2Component>(*eem) == 1);
}

TEST_CASE("Add component after entity destruction", "[ecs]") {
    CREATE_NEW_EEM(eem);

    Entity entity = CreateEntity(eem);
    REQUIRE(AddComponent(eem, entity, EEntityComponentType::Test2));

    DestroyEntity(eem, entity);

    // Component should be removed when entity is destroyed
    REQUIRE(GetComponentCount<Test2Component>(*eem) == 0);
    REQUIRE(GetComponentIndex<Test2Component>(*eem, entity) == NONE);

    // Adding component to destroyed entity should fail
    REQUIRE_FALSE(AddComponent(eem, entity, EEntityComponentType::Test2));
}

TEST_CASE("Add components to multiple entities", "[ecs]") {
    CREATE_NEW_EEM(eem);

    std::array<Entity, 3> entities;
    for (auto& entity : entities) {
        entity = CreateEntity(eem);
        REQUIRE(AddComponent(eem, entity, EEntityComponentType::Test2));
    }

    // Verify all entities have components
    for (const auto& entity : entities) {
        auto* signature = GetEntitySignature(eem, entity);
        REQUIRE(signature);
        REQUIRE(Matches(*signature, EEntityComponentType::Test2));
        REQUIRE(GetComponentIndex<Test2Component>(*eem, entity) != NONE);
    }

    REQUIRE(GetComponentCount<Test2Component>(*eem) == 3);
}

TEST_CASE("Remove component from valid entity", "[ecs]") {
    CREATE_NEW_EEM(eem);

    Entity entity = CreateEntity(eem);
    REQUIRE(AddComponent(eem, entity, EEntityComponentType::Test2));
    REQUIRE(RemoveComponent(eem, entity, EEntityComponentType::Test2));

    {
        auto* signature = GetEntitySignature(eem, entity);
        REQUIRE(signature);

        REQUIRE_FALSE(Matches(*signature, EEntityComponentType::Test2));
        REQUIRE(GetComponentCount<Test2Component>(*eem) == 0);
        REQUIRE(GetComponentIndex<Test2Component>(*eem, entity) == NONE);
    }
}

TEST_CASE("Remove component from invalid entity", "[ecs]") {
    CREATE_NEW_EEM(eem);

    // Try to remove component from NONE entity
    REQUIRE_FALSE(RemoveComponent(eem, NONE, EEntityComponentType::Test2));

    // Try to remove component from destroyed entity
    Entity entity = CreateEntity(eem);
    REQUIRE(AddComponent(eem, entity, EEntityComponentType::Test2));
    DestroyEntity(eem, entity);
    REQUIRE_FALSE(RemoveComponent(eem, entity, EEntityComponentType::Test2));

    // Try to remove component from entity with wrong generation
    Entity new_entity = CreateEntity(eem);  // Reuses the same slot
    REQUIRE(AddComponent(eem, new_entity, EEntityComponentType::Test2));
    Entity wrong_gen_entity =
        BuildEntity(GetEntityIndex(new_entity), GetEntityGeneration(new_entity) - 1);
    REQUIRE_FALSE(RemoveComponent(eem, wrong_gen_entity, EEntityComponentType::Test2));

    // The new entity should work
    REQUIRE(RemoveComponent(eem, new_entity, EEntityComponentType::Test2));
}

TEST_CASE("Remove non-existent component", "[ecs]") {
    CREATE_NEW_EEM(eem);

    Entity entity = CreateEntity(eem);

    // Try to remove component that was never added
    REQUIRE_FALSE(RemoveComponent(eem, entity, EEntityComponentType::Test2));

    // Add and remove the component
    REQUIRE(AddComponent(eem, entity, EEntityComponentType::Test2));
    REQUIRE(RemoveComponent(eem, entity, EEntityComponentType::Test2));

    // Try to remove it again
    REQUIRE_FALSE(RemoveComponent(eem, entity, EEntityComponentType::Test2));
}

TEST_CASE("Remove components from multiple entities", "[ecs]") {
    using namespace kdk::ecs_entity_test_private;

    CREATE_NEW_EEM(eem);

    std::array<Entity, 3> entities;
    for (auto& entity : entities) {
        entity = CreateEntity(eem);
        REQUIRE(AddComponent(eem, entity, EEntityComponentType::Test2));
    }

    // Remove component from middle entity
    REQUIRE(RemoveComponent(eem, entities[1], EEntityComponentType::Test2));
    REQUIRE(GetComponentCount<Test2Component>(*eem) == 2);

    VerifyEntityComponentMatch(eem, entities[0], Test2Component::kComponentType, true);
    REQUIRE(GetComponentIndex<Test2Component>(*eem, entities[1]) == NONE);
    VerifyEntityComponentMatch(eem, entities[2], Test2Component::kComponentType, true);

    // Remove remaining components
    REQUIRE(RemoveComponent(eem, entities[0], EEntityComponentType::Test2));
    REQUIRE(GetComponentCount<Test2Component>(*eem) == 1);
    REQUIRE(GetComponentIndex<Test2Component>(*eem, entities[0]) == NONE);
    REQUIRE(GetComponentIndex<Test2Component>(*eem, entities[1]) == NONE);
    VerifyEntityComponentMatch(eem, entities[2], Test2Component::kComponentType, true);

    REQUIRE(RemoveComponent(eem, entities[2], EEntityComponentType::Test2));
    REQUIRE(GetComponentCount<Test2Component>(*eem) == 0);
    REQUIRE(GetComponentIndex<Test2Component>(*eem, entities[0]) == NONE);
    REQUIRE(GetComponentIndex<Test2Component>(*eem, entities[1]) == NONE);
    REQUIRE(GetComponentIndex<Test2Component>(*eem, entities[2]) == NONE);
}

TEST_CASE("Add and remove multiple components", "[ecs]") {
    using namespace kdk::ecs_entity_test_private;

    CREATE_NEW_EEM(eem);

    Entity entity = CreateEntity(eem);

    // Add both components
    REQUIRE(AddComponent(eem, entity, EEntityComponentType::Test2));
    REQUIRE(AddComponent(eem, entity, EEntityComponentType::Test));

    {
        INFO("Verify both components are present");
        auto* signature = GetEntitySignature(eem, entity);
        REQUIRE(signature);
        REQUIRE(GetComponentCount<Test2Component>(*eem) == 1);
        REQUIRE(GetComponentCount<TestComponent>(*eem) == 1);
        VerifyEntityComponentMatch<Test2Component>(eem, entity, true);
        VerifyEntityComponentMatch<TestComponent>(eem, entity, true);
    }

    {
        INFO("Remove one component");
        REQUIRE(RemoveComponent(eem, entity, EEntityComponentType::Test2));

        auto* signature = GetEntitySignature(eem, entity);
        REQUIRE(signature);
        REQUIRE(GetComponentCount<Test2Component>(*eem) == 0);
        REQUIRE(GetComponentCount<TestComponent>(*eem) == 1);
        VerifyEntityComponentMatch<Test2Component>(eem, entity, false);
        VerifyEntityComponentMatch<TestComponent>(eem, entity, true);
    }

    {
        INFO("Verify all components are removed");
        REQUIRE(RemoveComponent(eem, entity, EEntityComponentType::Test));
        auto* signature = GetEntitySignature(eem, entity);

        REQUIRE(signature);
        REQUIRE(GetComponentCount<Test2Component>(*eem) == 0);
        REQUIRE(GetComponentCount<TestComponent>(*eem) == 0);
        VerifyEntityComponentMatch<Test2Component>(eem, entity, false);
        VerifyEntityComponentMatch<TestComponent>(eem, entity, false);
    }
}

TEST_CASE("Multiple entities with different component combinations", "[ecs]") {
    using namespace kdk::ecs_entity_test_private;

    CREATE_NEW_EEM(eem);

    Entity e1 = CreateEntity(eem);  // Will have both components
    Entity e2 = CreateEntity(eem);  // Will have only Test2
    Entity e3 = CreateEntity(eem);  // Will have only Test

    {
        INFO("Add components in different combinations");
        REQUIRE(AddComponent(eem, e1, EEntityComponentType::Test2));
        REQUIRE(AddComponent(eem, e1, EEntityComponentType::Test));
        REQUIRE(AddComponent(eem, e2, EEntityComponentType::Test2));
        REQUIRE(AddComponent(eem, e3, EEntityComponentType::Test));

        REQUIRE(GetComponentCount<Test2Component>(*eem) == 2);
        REQUIRE(GetComponentCount<TestComponent>(*eem) == 2);

        VerifyEntityComponentMatch<Test2Component>(eem, e1, true);
        VerifyEntityComponentMatch<TestComponent>(eem, e1, true);
        VerifyEntityComponentMatch<Test2Component>(eem, e2, true);
        VerifyEntityComponentMatch<TestComponent>(eem, e2, false);
        VerifyEntityComponentMatch<Test2Component>(eem, e3, false);
        VerifyEntityComponentMatch<TestComponent>(eem, e3, true);
    }

    {
        INFO("Remove components in mixed order");
        REQUIRE(RemoveComponent(eem, e1, EEntityComponentType::Test));
        REQUIRE(RemoveComponent(eem, e2, EEntityComponentType::Test2));
        REQUIRE(RemoveComponent(eem, e3, EEntityComponentType::Test));

        REQUIRE(GetComponentCount<Test2Component>(*eem) == 1);
        REQUIRE(GetComponentCount<TestComponent>(*eem) == 0);

        VerifyEntityComponentMatch<Test2Component>(eem, e1, true);
        VerifyEntityComponentMatch<TestComponent>(eem, e1, false);
        VerifyEntityComponentMatch<Test2Component>(eem, e2, false);
        VerifyEntityComponentMatch<TestComponent>(eem, e2, false);
        VerifyEntityComponentMatch<Test2Component>(eem, e3, false);
        VerifyEntityComponentMatch<TestComponent>(eem, e3, false);
    }
}

TEST_CASE("Entity destruction with multiple components", "[ecs]") {
    using namespace kdk::ecs_entity_test_private;

    CREATE_NEW_EEM(eem);

    Entity entity = CreateEntity(eem);

    // Add both components
    REQUIRE(AddComponent(eem, entity, EEntityComponentType::Test2));
    REQUIRE(AddComponent(eem, entity, EEntityComponentType::Test));
    REQUIRE(GetComponentCount<Test2Component>(*eem) == 1);
    REQUIRE(GetComponentCount<TestComponent>(*eem) == 1);

    {
        INFO("Verify initial state");
        VerifyEntityComponentMatch<Test2Component>(eem, entity, true);
        VerifyEntityComponentMatch<TestComponent>(eem, entity, true);
    }

    {
        INFO("Destroy entity");
        DestroyEntity(eem, entity);

        REQUIRE(GetComponentCount<Test2Component>(*eem) == 0);
        REQUIRE(GetComponentCount<TestComponent>(*eem) == 0);
    }
}
