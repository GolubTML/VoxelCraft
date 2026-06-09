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
#include <core/garbageCollector.hpp>

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
class Renderer;

enum BiomeType : uint8_t
{
    Forest,
    Desert,
    Plains,
    Tundra
};

class World
{
public:
    World(int seed);
    ~World();

    void initWorldThread(Device& device);
    void cleanup(VkDevice device);

    void updatePlayerPos(const glm::vec3& playerPos);
    void uploadChunksToGpu(const Renderer& renderer, GarbageCollector& gc, uint32_t currentFrame);

    void generateChunks(const glm::ivec3& chunkPos);
    
    const int getWorldSeed() const;
    const std::map<glm::ivec3, std::unique_ptr<Chunk>, ChunkPosCompare>& getChunks() const;
    std::mutex& getChunkMutex() const;
    
    BlockType getBlock(const glm::ivec3& globalPos) const;
    void setBlock(const glm::ivec3& globalPos, BlockType type);
    
private:
    Device* devicePtr = nullptr;
    
    fnl_state noise;

    fnl_state tempNoise;
    fnl_state moistureNoise;

    fnl_state oceanNoise;
    fnl_state riverNoise;

    int worldSeed;
    
    std::thread generationThread;
    mutable std::mutex chunksMutex;
    std::atomic<bool> isRunning{false};
    
    std::atomic<int> playerChunkX{0};
    std::atomic<int> playerChunkZ{0};
    
    std::map<glm::ivec3, std::unique_ptr<Chunk>, ChunkPosCompare> chunks;
    
    std::pair<std::vector<Vertex>, std::vector<uint32_t>> generateMeshData(Chunk& chunk);
    
    void threadLoop();
    void saveChunkToFile(const glm::ivec3& pos, const Chunk& chunk);
    bool loadChunkFromFile(const glm::ivec3& pos, Chunk& chunk);

    void generateCactuses(Chunk& chunk, int x, int z);
    void generateTrees(Chunk& chunk, int x, int z);
    void generateFlowers(Chunk& chunk, int x, int z);
    
    BlockType calculateBlockType(int globalX, int globalY, int globalZ);

    BlockType getBlockAt(const glm::ivec3& globalPos) const;

    BiomeType getBiomeAt(int globalX, int globalZ);

    int findSurfaceHight(const Chunk& chunk, int x, int z);
    int getVertexAO(const glm::ivec3& blockPos, const glm::ivec3& normal, 
        const glm::ivec3& edge1, const glm::ivec3& edge2,
        const Chunk& chunk, 
        Chunk* nXPlus, Chunk* nXMinus, Chunk* nZPlus, Chunk* nZMinus);

    std::vector<GarbageCollector::GarbageBuffer> clearQueue;
};