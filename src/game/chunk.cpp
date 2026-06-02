#include <game/chunk.hpp>

void Chunk::cleanup(VkDevice device)
{
    mesh.cleanup(device);
}