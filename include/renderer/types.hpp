#pragma once

// i really don't know, where to put this structure

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct UniformBufferObject
{
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};