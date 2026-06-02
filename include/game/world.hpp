#pragma once

#include <map>
#include <memory>

#include <lib/FastNoiseLite.h>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <game/block.hpp>
#include <game/chunk.hpp>

struct ChunkPosCompare 
{
    bool operator()(const glm::ivec3& a, const glm::ivec3& b) const 
    {
        if (a.x != b.x) return a.x < b.x;
        if (a.y != b.y) return a.y < b.y;

        return a.z < b.z;
    }
};

class Chunk;
class Device;

class World
{
public:
    World(int seed);
    ~World();

    void create(Device& device);
    void cleanup(VkDevice device);

    void generateChunks(const glm::ivec3& chunkPos);
    void generateMeshForChunks(Device& device, Chunk& chunk);

    BlockType getBlockAt(const glm::ivec3& globalPos) const;

    const std::map<glm::ivec3, std::unique_ptr<Chunk>, ChunkPosCompare>& getChunks() const;

private:
    fnl_state noise;
    int worldSeed;

    std::map<glm::ivec3, std::unique_ptr<Chunk>, ChunkPosCompare> chunks;

    BlockType calculateBlockType(int globalX, int globalY, int globalZ);
};