#include <catch2/catch_test_macros.hpp>

#include <kandinsky/entity.h>

using namespace kdk;

namespace kdk::ecs_entity_test_private {

void VerifyEntityComponentMatch(EntityManager* eem,
                                EntityID id,
                                EEntityComponentType component_type,
                                bool should_match) {
    INFO("Entity: " << id.GetIndex() << ", COMPONENT: " << ToString(component_type));

    auto* signature = GetEntitySignature(eem, id);
    REQUIRE(signature);
    if (should_match) {
        REQUIRE(Matches(*signature, component_type));
        EntityComponentIndex component_index = GetComponentIndex(*eem, id, component_type);
        REQUIRE(component_index != NONE);
        REQUIRE(GetOwningEntity(*eem, component_type, component_index) == id);
    } else {
        REQUIRE_FALSE(Matches(*signature, component_type));
        EntityComponentIndex component_index = GetComponentIndex(*eem, id, component_type);
        REQUIRE(component_index == NONE);
    }
}

template <typename T>
void VerifyEntityComponentMatch(EntityManager* eem, EntityID id, bool should_match) {
    return VerifyEntityComponentMatch(eem, id, T::kComponentType, should_match);
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

    auto [id, entity] = CreateEntity(eem);
    REQUIRE(id == entity->ID);
    REQUIRE(id.GetIndex() == 0);
    REQUIRE(id.GetGeneration() == 1);
    REQUIRE(eem->EntityCount == 1);
    REQUIRE(eem->NextIndex == 1);
    REQUIRE(eem->Signatures[id.GetIndex()] == kNewEntitySignature);
}

TEST_CASE("ECS Entity Creation and Destruction: Create multiple entities", "[ecs]") {
    CREATE_NEW_EEM(eem);

    auto [id1, entity1] = CreateEntity(eem);
    REQUIRE(eem->NextIndex == 1);
    auto [id2, entity2] = CreateEntity(eem);
    REQUIRE(eem->NextIndex == 2);
    auto [id3, entity3] = CreateEntity(eem);
    REQUIRE(eem->NextIndex == 3);

    REQUIRE(eem->EntityCount == 3);
    REQUIRE(id1.GetIndex() == 0);
    REQUIRE(id1.GetGeneration() == 1);
    REQUIRE(id2.GetIndex() == 1);
    REQUIRE(id2.GetGeneration() == 1);
    REQUIRE(id3.GetIndex() == 2);
    REQUIRE(id3.GetGeneration() == 1);
}

TEST_CASE("ECS Entity Creation and Destruction: Destroy entity and create new one", "[ecs]") {
    CREATE_NEW_EEM(eem);

    auto [id1, entity1] = CreateEntity(eem);
    REQUIRE(eem->EntityCount == 1);
    REQUIRE(eem->NextIndex == 1);
    REQUIRE(id1.GetIndex() == 0);
    REQUIRE(id1.GetGeneration() == 1);

    DestroyEntity(eem, id1);
    REQUIRE(eem->EntityCount == 0);
    REQUIRE(eem->NextIndex == 0);
    REQUIRE(id1.GetIndex() == 0);
    REQUIRE(id1.GetGeneration() == 1);

    // Creating a new entity should reuse the destroyed slot
    auto [id2, entity2] = CreateEntity(eem);
    REQUIRE(eem->EntityCount == 1);
    REQUIRE(eem->NextIndex == 1);
    REQUIRE(id2.GetIndex() == 0);
    REQUIRE(id2.GetGeneration() == 2);
}

TEST_CASE("ECS Entity Creation and Destruction: Create and destroy multiple entities", "[ecs]") {
    CREATE_NEW_EEM(eem);

    std::array<EntityID, 5> entities;
    for (u32 i = 0; i < entities.size(); ++i) {
        std::tie(entities[i], std::ignore) = CreateEntity(eem);
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

    EntityID new_id = {};

    // Next created entity should use the freed slot
    std::tie(new_id, std::ignore) = CreateEntity(eem);
    REQUIRE(new_id.GetIndex() == 4);
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
    std::tie(new_id, std::ignore) = CreateEntity(eem);
    REQUIRE(new_id.GetIndex() == 1);
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
    std::tie(new_id, std::ignore) = CreateEntity(eem);
    REQUIRE(new_id.GetIndex() == 5);
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

    auto [id, entity] = CreateEntity(eem);
    auto [component_index, component] = AddComponent<Test2Component>(eem, id);
    REQUIRE(component_index != NONE);
    REQUIRE(component != nullptr);
    REQUIRE(component->GetOwnerID() == id);
    REQUIRE(component->GetComponentIndex() == component_index);

    {
        auto* signature = GetEntitySignature(eem, id);
        REQUIRE(signature);

        REQUIRE(Matches(*signature, EEntityComponentType::Test2));
        REQUIRE(GetComponentCount<Test2Component>(*eem) == 1);
        REQUIRE(GetComponentIndex<Test2Component>(*eem, id) == 0);
        REQUIRE(GetOwningEntity<Test2Component>(*eem, 0) == id);
    }
}

TEST_CASE("Add component to invalid entity", "[ecs]") {
    CREATE_NEW_EEM(eem);

    // Try to add component to NONE entity
    auto [component_index, component] = AddComponent<Test2Component>(eem, {});
    REQUIRE(component_index == NONE);
    REQUIRE(component == nullptr);

    // Try to add component to destroyed entity
    auto [id, entity] = CreateEntity(eem);
    DestroyEntity(eem, id);
    REQUIRE(AddComponentTest(eem, id, EEntityComponentType::Test2) == NONE);

    // Try to add component to entity with wrong generation
    auto [new_id, new_entity] = CreateEntity(eem);  // Reuses the same slot
    EntityID wrong_gen_entity_id = EntityID::Build(new_id.GetIndex(), new_id.GetGeneration() - 1);
    REQUIRE(AddComponentTest(eem, wrong_gen_entity_id, EEntityComponentType::Test2) == NONE);

    // The new entity should work.
    REQUIRE(AddComponentTest(eem, new_id, EEntityComponentType::Test2) != NONE);
}

TEST_CASE("Add same component multiple times", "[ecs]") {
    CREATE_NEW_EEM(eem);

    auto [id, entity] = CreateEntity(eem);

    // First addition should succeed
    REQUIRE(AddComponentTest(eem, id, EEntityComponentType::Test2) != NONE);

    // Second addition should fail
    REQUIRE(AddComponentTest(eem, id, EEntityComponentType::Test2) == NONE);

    // Component count should still be 1
    REQUIRE(GetComponentCount<Test2Component>(*eem) == 1);
}

TEST_CASE("Add component after entity destruction", "[ecs]") {
    CREATE_NEW_EEM(eem);

    auto [id, entity] = CreateEntity(eem);
    REQUIRE(AddComponentTest(eem, id, EEntityComponentType::Test2) != NONE);

    DestroyEntity(eem, id);

    // Component should be removed when id is destroyed
    REQUIRE(GetComponentCount<Test2Component>(*eem) == 0);
    REQUIRE(GetComponentIndex<Test2Component>(*eem, id) == NONE);

    // Adding component to destroyed entity should fail
    REQUIRE(AddComponentTest(eem, id, EEntityComponentType::Test2) == NONE);
}

TEST_CASE("Add components to multiple entities", "[ecs]") {
    CREATE_NEW_EEM(eem);

    std::array<EntityID, 3> entities;
    for (auto& id : entities) {
        std::tie(id, std::ignore) = CreateEntity(eem);
        REQUIRE(AddComponentTest(eem, id, EEntityComponentType::Test2) != NONE);
    }

    // Verify all entities have components
    for (const auto& id : entities) {
        auto* signature = GetEntitySignature(eem, id);
        REQUIRE(signature);
        REQUIRE(Matches(*signature, EEntityComponentType::Test2));
        REQUIRE(GetComponentIndex<Test2Component>(*eem, id) != NONE);
    }

    REQUIRE(GetComponentCount<Test2Component>(*eem) == 3);
}

TEST_CASE("Remove component from valid entity", "[ecs]") {
    CREATE_NEW_EEM(eem);

    auto [id, entity] = CreateEntity(eem);
    REQUIRE(AddComponentTest(eem, id, EEntityComponentType::Test2) != NONE);
    REQUIRE(RemoveComponent(eem, id, EEntityComponentType::Test2));

    {
        auto* signature = GetEntitySignature(eem, id);
        REQUIRE(signature);

        REQUIRE_FALSE(Matches(*signature, EEntityComponentType::Test2));
        REQUIRE(GetComponentCount<Test2Component>(*eem) == 0);
        REQUIRE(GetComponentIndex<Test2Component>(*eem, id) == NONE);
    }
}

TEST_CASE("Remove component from invalid entity", "[ecs]") {
    CREATE_NEW_EEM(eem);

    // Try to remove component from NONE entity
    REQUIRE_FALSE(RemoveComponent(eem, {}, EEntityComponentType::Test2));

    // Try to remove component from destroyed entity
    auto [id, entity] = CreateEntity(eem);
    REQUIRE(AddComponentTest(eem, id, EEntityComponentType::Test2) != NONE);
    DestroyEntity(eem, id);
    REQUIRE_FALSE(RemoveComponent(eem, id, EEntityComponentType::Test2));

    // Try to remove component from entity with wrong generation
    auto [new_id, new_entity] = CreateEntity(eem);  // Reuses the same slot
    REQUIRE(AddComponentTest(eem, new_id, EEntityComponentType::Test2) != NONE);
    EntityID wrong_gen_entity = EntityID::Build(new_id.GetIndex(), new_id.GetGeneration() - 1);
    REQUIRE_FALSE(RemoveComponent(eem, wrong_gen_entity, EEntityComponentType::Test2));

    // The new entity should work
    REQUIRE(RemoveComponent(eem, new_id, EEntityComponentType::Test2));
}

TEST_CASE("Remove non-existent component", "[ecs]") {
    CREATE_NEW_EEM(eem);

    auto [id, entity] = CreateEntity(eem);

    // Try to remove component that was never added
    REQUIRE_FALSE(RemoveComponent(eem, id, EEntityComponentType::Test2));

    // Add and remove the component
    REQUIRE(AddComponentTest(eem, id, EEntityComponentType::Test2) != NONE);
    REQUIRE(RemoveComponent(eem, id, EEntityComponentType::Test2));

    // Try to remove it again
    REQUIRE_FALSE(RemoveComponent(eem, id, EEntityComponentType::Test2));
}

TEST_CASE("Remove components from multiple entities", "[ecs]") {
    using namespace kdk::ecs_entity_test_private;

    CREATE_NEW_EEM(eem);

    std::array<EntityID, 3> entities;
    for (auto& id : entities) {
        std::tie(id, std::ignore) = CreateEntity(eem);
        REQUIRE(AddComponentTest(eem, id, EEntityComponentType::Test2) != NONE);
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

    auto [id, entity] = CreateEntity(eem);

    // Add both components
    REQUIRE(AddComponentTest(eem, id, EEntityComponentType::Test) != NONE);
    REQUIRE(AddComponentTest(eem, id, EEntityComponentType::Test2) != NONE);

    {
        INFO("Verify both components are present");
        auto* signature = GetEntitySignature(eem, id);
        REQUIRE(signature);
        REQUIRE(GetComponentCount<Test2Component>(*eem) == 1);
        REQUIRE(GetComponentCount<TestComponent>(*eem) == 1);
        VerifyEntityComponentMatch<Test2Component>(eem, id, true);
        VerifyEntityComponentMatch<TestComponent>(eem, id, true);
    }

    {
        INFO("Remove one component");
        REQUIRE(RemoveComponent(eem, id, EEntityComponentType::Test2));

        auto* signature = GetEntitySignature(eem, id);
        REQUIRE(signature);
        REQUIRE(GetComponentCount<Test2Component>(*eem) == 0);
        REQUIRE(GetComponentCount<TestComponent>(*eem) == 1);
        VerifyEntityComponentMatch<Test2Component>(eem, id, false);
        VerifyEntityComponentMatch<TestComponent>(eem, id, true);
    }

    {
        INFO("Verify all components are removed");
        REQUIRE(RemoveComponent(eem, id, EEntityComponentType::Test));
        auto* signature = GetEntitySignature(eem, id);

        REQUIRE(signature);
        REQUIRE(GetComponentCount<Test2Component>(*eem) == 0);
        REQUIRE(GetComponentCount<TestComponent>(*eem) == 0);
        VerifyEntityComponentMatch<Test2Component>(eem, id, false);
        VerifyEntityComponentMatch<TestComponent>(eem, id, false);
    }
}

TEST_CASE("Multiple entities with different component combinations", "[ecs]") {
    using namespace kdk::ecs_entity_test_private;

    CREATE_NEW_EEM(eem);

    auto [e1, entity1] = CreateEntity(eem);  // Will have both components
    auto [e2, entity2] = CreateEntity(eem);  // Will have only Test2
    auto [e3, entity3] = CreateEntity(eem);  // Will have only Test

    {
        INFO("Add components in different combinations");
        REQUIRE(AddComponentTest(eem, e1, EEntityComponentType::Test2) != NONE);
        REQUIRE(AddComponentTest(eem, e1, EEntityComponentType::Test) != NONE);
        REQUIRE(AddComponentTest(eem, e2, EEntityComponentType::Test2) != NONE);
        REQUIRE(AddComponentTest(eem, e3, EEntityComponentType::Test) != NONE);

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

    auto [id, entity] = CreateEntity(eem);

    // Add both components
    REQUIRE(AddComponentTest(eem, id, EEntityComponentType::Test2) != NONE);
    REQUIRE(AddComponentTest(eem, id, EEntityComponentType::Test) != NONE);
    REQUIRE(GetComponentCount<Test2Component>(*eem) == 1);
    REQUIRE(GetComponentCount<TestComponent>(*eem) == 1);

    {
        INFO("Verify initial state");
        VerifyEntityComponentMatch<Test2Component>(eem, id, true);
        VerifyEntityComponentMatch<TestComponent>(eem, id, true);
    }

    {
        INFO("Destroy entity");
        DestroyEntity(eem, id);

        REQUIRE(GetComponentCount<Test2Component>(*eem) == 0);
        REQUIRE(GetComponentCount<TestComponent>(*eem) == 0);
    }
}
