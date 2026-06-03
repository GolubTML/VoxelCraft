#pragma once

#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

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

    void initWorldThread(Device& device);
    void cleanup(VkDevice device);

    void updatePlayerPos(const glm::vec3& playerPos);

    void generateChunks(const glm::ivec3& chunkPos);
    void generateMeshForChunks(Device& device, Chunk& chunk);

    BlockType getBlockAt(const glm::ivec3& globalPos) const;

    const std::map<glm::ivec3, std::unique_ptr<Chunk>, ChunkPosCompare>& getChunks() const;

    std::mutex& getChunkMutex() const;

private:
    Device* devicePtr = nullptr;

    fnl_state noise;
    int worldSeed;

    std::thread generationThread;
    mutable std::mutex chunksMutex;
    std::atomic<bool> isRunning{false};

    std::atomic<int> playerChunkX{0};
    std::atomic<int> playerChunkZ{0};

    std::map<glm::ivec3, std::unique_ptr<Chunk>, ChunkPosCompare> chunks;

    void threadLoop();

    BlockType calculateBlockType(int globalX, int globalY, int globalZ);
};