#pragma once

#include <kandinsky/ecs/ecs_entity.h>
#include <kandinsky/math.h>


namespace kdk {

struct TransformComponent
{
	static constexpr EComponents Type = EComponents::Transform;

	Transform Transform = {};
};


} // namespace kdk
