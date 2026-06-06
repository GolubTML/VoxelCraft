#define FNL_IMPL
#include <game/world.hpp>
#include <game/chunk.hpp>
#include <core/device.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <renderer/renderer.hpp>

World::World(int seed) : worldSeed(seed)
{
    noise = fnlCreateState();
    noise.seed = worldSeed;
    noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    noise.fractal_type = FNL_FRACTAL_FBM;
    noise.octaves = 4;
    noise.frequency = 0.005f;

    tempNoise = fnlCreateState();
    tempNoise.seed = worldSeed + 100;
    tempNoise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    tempNoise.fractal_type = FNL_FRACTAL_FBM;
    tempNoise.frequency = 0.0008f;

    moistureNoise = fnlCreateState();
    moistureNoise.seed = worldSeed + 200;
    moistureNoise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    moistureNoise.fractal_type = FNL_FRACTAL_FBM;
    moistureNoise.frequency = 0.0008f;

    oceanNoise = fnlCreateState();
    oceanNoise.seed = worldSeed + 300;
    oceanNoise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    oceanNoise.fractal_type = FNL_FRACTAL_FBM;
    oceanNoise.frequency = 0.001f;

    riverNoise = fnlCreateState();
    riverNoise.seed = worldSeed + 400;
    riverNoise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    riverNoise.fractal_type = FNL_FRACTAL_FBM;
    riverNoise.frequency = 0.002f;

    std::filesystem::path worldSaveDir = "world_save";

    try
    {
        std::filesystem::remove_all(worldSaveDir); 
        std::cout << "Folder " << worldSaveDir << " was deleted!" << "\n";
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cout << "Error was found, while deleting files in 'world_save' folder: " << e.what() << "\n";
    }
}

World::~World() 
{ 
    devicePtr = nullptr;
    isRunning = false;

    if (generationThread.joinable())
        generationThread.join();
}

void World::initWorldThread(Device& device)
{
    devicePtr = &device;
    isRunning = true;

    generationThread = std::thread(&World::threadLoop, this);
}

void World::updatePlayerPos(const glm::vec3& playerPos)
{
    if (!isRunning) return;

    int cx = static_cast<int>(playerPos.x) / Chunk::WIDTH;
    int cz = static_cast<int>(playerPos.z) / Chunk::LENGTH;

    if (playerPos.x < 0) cx--;
    if (playerPos.z < 0) cz--;

    playerChunkX = cx;
    playerChunkZ = cz;
}

void World::uploadChunksToGpu(const Renderer& renderer, GarbageCollector& gc, uint32_t currentFrame)
{
    std::lock_guard<std::mutex> lock(chunksMutex);

    for (const auto& dead : clearQueue)
    {
        gc.pushBuffer(dead.buffer, dead.memory, currentFrame);
    }
    
    clearQueue.clear();

    for (auto& [pos, chunk] : chunks)
    {
        if (chunk && chunk->hasNewMeshData)
        {
            chunk->mesh.create(*devicePtr, chunk->tempVertices, chunk->tempIndices, gc, currentFrame);

            chunk->tempVertices.clear();
            chunk->tempIndices.clear();
            chunk->hasNewMeshData = false;
        }
    }
}

void World::cleanup(VkDevice device)
{
    isRunning = false;

    if (generationThread.joinable())
        generationThread.join();

    for (auto& [pos, chunk] : chunks)
        if (chunk)   
            chunk->cleanup(device);

    std::filesystem::path worldSaveDir = "world_save";

    try
    {
        std::filesystem::remove_all(worldSaveDir); 
        std::cout << "Folder " << worldSaveDir << " was deleted!" << "\n";
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cout << "Error was found, while deleting files in 'world_save' folder: " << e.what() << "\n";
    }

    chunks.clear();
}

void World::generateChunks(const glm::ivec3& chunkPos)
{
    if (chunks.find(chunkPos) != chunks.end()) return;

    auto newChunk = std::make_unique<Chunk>();
    newChunk->pos = chunkPos;

    if (!loadChunkFromFile(chunkPos, *newChunk))
    {
        for (int x = 0; x < Chunk::WIDTH; ++x)
            for (int y = 0; y < Chunk::HEIGHT; ++y)
                for (int z = 0; z < Chunk::LENGTH; ++z)
                {
                    int globalX = chunkPos.x * Chunk::WIDTH + x;
                    int globalY = chunkPos.y * Chunk::HEIGHT + y;
                    int globalZ = chunkPos.z * Chunk::LENGTH + z;

                    newChunk->blocks[x][y][z].type = calculateBlockType(globalX, globalY, globalZ);
                }

        for (int x = 0; x < Chunk::WIDTH; ++x)
            for (int z = 0; z < Chunk::LENGTH; ++z)
            {
                generateTrees(*newChunk, x, z);
                generateCactuses(*newChunk, x, z);
                generateFlowers(*newChunk, x, z);
            }
    }
    else
        std::cout << "Chunk loaded from memory!" << "\n";

    chunks[chunkPos] = std::move(newChunk);
}

std::pair<std::vector<Vertex>, std::vector<uint32_t>> World::generateMeshData(Chunk& chunk)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // lamda function, we will need this
    auto addFace = [&](glm::vec3 pos, glm::vec3 color, BlockFace face, BlockUV uv)
    {
        uint32_t start = vertices.size();

        float x0 = uv.topLeft.x;
        float y0 = uv.topLeft.y;
        float x1 = uv.bottomRight.x;
        float y1 = uv.bottomRight.y;

        switch (face)
        {
        case BlockFace::TOP:
            vertices.push_back({pos + glm::vec3(0,1,0), color, {x1, y0}});
            vertices.push_back({pos + glm::vec3(1,1,0), color, {x0, y0}});
            vertices.push_back({pos + glm::vec3(1,1,1), color, {x0, y1}});
            vertices.push_back({pos + glm::vec3(0,1,1), color, {x1, y1}});

            break;
        case BlockFace::BOTTOM:
            vertices.push_back({pos + glm::vec3(0,0,0), color, {x1, y0}});
            vertices.push_back({pos + glm::vec3(1,0,0), color, {x0, y0}});
            vertices.push_back({pos + glm::vec3(1,0,1), color, {x0, y1}});
            vertices.push_back({pos + glm::vec3(0,0,1), color, {x1, y1}});

            break;
        case BlockFace::FRONT:
            vertices.push_back({pos + glm::vec3(0,0,1), color, {x1, y0}});
            vertices.push_back({pos + glm::vec3(1,0,1), color, {x0, y0}});
            vertices.push_back({pos + glm::vec3(1,1,1), color, {x0, y1}});
            vertices.push_back({pos + glm::vec3(0,1,1), color, {x1, y1}});
            
            break;
        case BlockFace::BACK:
            vertices.push_back({pos + glm::vec3(0,0,0), color, {x1, y0}});
            vertices.push_back({pos + glm::vec3(1,0,0), color, {x0, y0}});
            vertices.push_back({pos + glm::vec3(1,1,0), color, {x0, y1}});
            vertices.push_back({pos + glm::vec3(0,1,0), color, {x1, y1}});
            
            break;
        case BlockFace::LEFT:
            vertices.push_back({pos + glm::vec3(0,0,0), color, {x1, y0}});
            vertices.push_back({pos + glm::vec3(0,0,1), color, {x0, y0}});
            vertices.push_back({pos + glm::vec3(0,1,1), color, {x0, y1}});
            vertices.push_back({pos + glm::vec3(0,1,0), color, {x1, y1}});

            break;
        case BlockFace::RIGHT:
            vertices.push_back({pos + glm::vec3(1,0,0), color, {x1, y0}});
            vertices.push_back({pos + glm::vec3(1,0,1), color, {x0, y0}});
            vertices.push_back({pos + glm::vec3(1,1,1), color, {x0, y1}});
            vertices.push_back({pos + glm::vec3(1,1,0), color, {x1, y1}});

            break;
        
        default:
            break;
        }

        indices.insert(indices.end(), {
            start + 0, start + 1, start + 2,
            start + 2, start + 3, start + 0
        });
    };

    auto addCrossFaces = [&](glm::vec3 pos, glm::vec3 color, BlockUV uv)
    {
        uint32_t start = vertices.size();

        float x0 = uv.topLeft.x;
        float y0 = uv.topLeft.y;
        float x1 = uv.bottomRight.x;
        float y1 = uv.bottomRight.y;

        vertices.push_back({pos + glm::vec3(0, 0, 0), color, {x1, y0}});
        vertices.push_back({pos + glm::vec3(1, 0, 1), color, {x0, y0}});
        vertices.push_back({pos + glm::vec3(1, 1, 1), color, {x0, y1}});
        vertices.push_back({pos + glm::vec3(0, 1, 0), color, {x1, y1}});

        vertices.push_back({pos + glm::vec3(1, 0, 0), color, {x1, y0}});
        vertices.push_back({pos + glm::vec3(0, 0, 1), color, {x0, y0}});
        vertices.push_back({pos + glm::vec3(0, 1, 1), color, {x0, y1}});
        vertices.push_back({pos + glm::vec3(1, 1, 0), color, {x1, y1}});

        for (uint32_t i = 0; i < 2; ++i) 
        {
            uint32_t offset = start + i * 4;
            indices.insert(indices.end(), 
            {
                offset + 0, offset + 1, offset + 2,
                offset + 2, offset + 3, offset + 0
            });
        }
    };

    for (int x = 0; x < Chunk::WIDTH; ++x)
        for (int y = 0; y < Chunk::HEIGHT; ++y)
            for (int z = 0; z < Chunk::LENGTH; ++z)
            {
                BlockType currentType = chunk.blocks[x][y][z].type;
                if (currentType == BlockType::Air)
                    continue;

                glm::vec3 localPos(x, y, z);

                if (isBlockCrossed(currentType))
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::FRONT); // we dont even have defenition of face here
                    glm::vec3 color = getBlockFaceColor(currentType, BlockFace::FRONT);

                    addCrossFaces(localPos, color, uv);
                    continue;
                }

                glm::ivec3 globalPos(chunk.pos.x * Chunk::WIDTH + x, chunk.pos.y * Chunk::HEIGHT + y, chunk.pos.z * Chunk::LENGTH + z);

                #pragma region Adding faces to cube mesh 
                // for each face, we need check for neighbour
                // +x axis

                BlockType neighborRight = getBlockAt(globalPos + glm::ivec3(1, 0, 0));
                if (neighborRight == BlockType::Air || (isBlockTransparent(neighborRight) && !isBlockTransparent(currentType)))
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::RIGHT);
                    glm::vec3 color = getBlockFaceColor(currentType, BlockFace::RIGHT);
                    addFace(localPos, color, BlockFace::RIGHT, uv);
                }
                
                // -x axis
                BlockType neighborLeft = getBlockAt(globalPos + glm::ivec3(-1, 0, 0));
                if (neighborLeft == BlockType::Air || (isBlockTransparent(neighborLeft) && !isBlockTransparent(currentType)))
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::LEFT);
                    glm::vec3 color = getBlockFaceColor(currentType, BlockFace::LEFT);
                    addFace(localPos, color, BlockFace::LEFT, uv);
                }

                // +y axis
                BlockType neighborTop = getBlockAt(globalPos + glm::ivec3(0, 1, 0));
                if (neighborTop == BlockType::Air || (isBlockTransparent(neighborTop) && !isBlockTransparent(currentType)))
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::TOP);
                    glm::vec3 color = getBlockFaceColor(currentType, BlockFace::TOP);
                    addFace(localPos, color, BlockFace::TOP, uv);
                }

                // -y axis
                BlockType neighborBottom = getBlockAt(globalPos + glm::ivec3(0, -1, 0));
                if (neighborBottom == BlockType::Air || (isBlockTransparent(neighborBottom) && !isBlockTransparent(currentType)))
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::BOTTOM);
                    glm::vec3 color = getBlockFaceColor(currentType, BlockFace::BOTTOM);
                    addFace(localPos, color, BlockFace::BOTTOM, uv);
                }

                // +z axis
                BlockType neighborFront = getBlockAt(globalPos + glm::ivec3(0, 0, 1));
                if (neighborFront == BlockType::Air || (isBlockTransparent(neighborFront) && !isBlockTransparent(currentType)))
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::FRONT);
                    glm::vec3 color = getBlockFaceColor(currentType, BlockFace::FRONT);
                    addFace(localPos, color, BlockFace::FRONT, uv);
                }

                // -z axis
                BlockType neighborBack = getBlockAt(globalPos + glm::ivec3(0, 0, -1));
                if (neighborBack == BlockType::Air || (isBlockTransparent(neighborBack) && !isBlockTransparent(currentType)))
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::BACK);
                    glm::vec3 color = getBlockFaceColor(currentType, BlockFace::BACK);
                    addFace(localPos, color, BlockFace::BACK, uv);
                }

                #pragma endregion
            } 

    chunk.modelMatrix = glm::translate(glm::mat4(1.0f), 
        glm::vec3(chunk.pos.x * Chunk::WIDTH, 
            chunk.pos.y * Chunk::HEIGHT, 
            chunk.pos.z * Chunk::LENGTH));

    return {vertices, indices};
}

const int World::getWorldSeed() const
{
    return worldSeed;
}

const std::map<glm::ivec3, std::unique_ptr<Chunk>, ChunkPosCompare>& World::getChunks() const
{
    return chunks;
}

std::mutex& World::getChunkMutex() const
{
    return chunksMutex;
}

void World::threadLoop()
{
    int renderRadius = 3;
    int unloadRadius = renderRadius + 2;

    while (isRunning)
    {
        int currentX = playerChunkX;
        int currentZ = playerChunkZ;

        for (int cx = currentX - renderRadius; cx <= currentX + renderRadius; ++cx)
            for (int cz = currentZ - renderRadius; cz <= currentZ + renderRadius; ++cz)        
            {
                if (!isRunning) return;

                glm::ivec3 pos(cx, 0, cz);

                bool chunkExists = false;
                {
                    std::lock_guard<std::mutex> lock(chunksMutex);
                    chunkExists = (chunks.find(pos) != chunks.end());
                }

                if (!chunkExists)
                {
                    generateChunks(pos);

                    {
                        std::lock_guard<std::mutex> lock(chunksMutex);
                        chunks[pos]->isGenerated = true;
                        chunks[pos]->needUpdate = true; // yes, because now it doesnt get face culling (because doesn't have any neighbors)

                        glm::ivec3 neighbors[] = 
                        {
                            pos + glm::ivec3(1, 0, 0), 
                            pos + glm::ivec3(-1, 0, 0),
                            pos + glm::ivec3(0, 0, 1), 
                            pos + glm::ivec3(0, 0, -1) 
                        };

                        for (const auto& neighbor : neighbors)
                        {
                            auto it = chunks.find(neighbor);

                            if (it != chunks.end() && it->second)
                            {
                                it->second->needUpdate = true; // also needs update
                            }
                        }
                    }
                }
            }

        {
            for (int cx = currentX - renderRadius; cx <= currentX + renderRadius; ++cx) 
                for (int cz = currentZ - renderRadius; cz <= currentZ + renderRadius; ++cz) 
                {
                    glm::ivec3 pos(cx, 0, cz);
                    Chunk* chunkToUpdate = nullptr;

                    {
                        std::lock_guard<std::mutex> lock(chunksMutex);
                        auto it = chunks.find(pos);

                        if (it != chunks.end() && it->second && it->second->needUpdate)
                            chunkToUpdate = it->second.get();
                    }

                    if (chunkToUpdate)
                    {
                        auto [vertices, indices] = generateMeshData(*chunkToUpdate);

                        {
                            std::lock_guard<std::mutex> lock(chunksMutex);

                            auto it = chunks.find(pos);
                            if (it != chunks.end() && it->second) 
                            {
                                if (!vertices.empty())
                                {
                                    it->second->tempVertices = std::move(vertices);
                                    it->second->tempIndices = std::move(indices);
                                    it->second->hasNewMeshData = true;
                                }

                                it->second->needUpdate = false;
                            }
                        }
                    }
                }
        }

        {
            std::lock_guard<std::mutex> lock(chunksMutex);

            for (auto it = chunks.begin(); it != chunks.end(); /*nothing*/)
            {
                glm::ivec3 itPos = it->first;

                int distanceX = std::abs(playerChunkX - itPos.x);
                int distanceZ = std::abs(playerChunkZ - itPos.z);

                if (distanceX > unloadRadius || distanceZ > unloadRadius)
                {
                    if (it->second)
                    {
                        saveChunkToFile(itPos, *(it->second));

                        if (it->second->mesh.vertexBuffer.buffer != VK_NULL_HANDLE) 
                        {
                            clearQueue.push_back({it->second->mesh.vertexBuffer.buffer, it->second->mesh.vertexBuffer.memory});
                        }
                        if (it->second->mesh.indexBuffer.buffer != VK_NULL_HANDLE) 
                        {
                            clearQueue.push_back({it->second->mesh.indexBuffer.buffer, it->second->mesh.indexBuffer.memory});
                        }
                    }

                    it = chunks.erase(it);
                }
                else
                {
                    it++;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void World::saveChunkToFile(const glm::ivec3& pos, const Chunk& chunk)
{
    std::filesystem::create_directories("world_save");

    const std::string fileName = "world_save/chunk_" + std::to_string(pos.x) + "_" + std::to_string(pos.y) + "_" + std::to_string(pos.z) + ".dat"; 

    std::ofstream out(fileName, std::ios::binary);
    if (!out.is_open()) return;

    const BlockType* blockPtr = &chunk.blocks[0][0][0].type;
    int totalBlocks = Chunk::WIDTH * Chunk::HEIGHT * Chunk::LENGTH;

    BlockType currentType = blockPtr[0];
    uint16_t runLength = 1;

    for (int i = 1; i < totalBlocks; ++i)
    {
        if (blockPtr[i] == currentType && runLength < UINT16_MAX)
        {
            ++runLength;
        }
        else
        {
            out.write(reinterpret_cast<const char*>(&runLength), sizeof(runLength));
            out.write(reinterpret_cast<const char*>(&currentType), sizeof(currentType));

            currentType = blockPtr[i];
            runLength = 1;
        }
    }

    out.write(reinterpret_cast<const char*>(&runLength), sizeof(runLength));
    out.write(reinterpret_cast<const char*>(&currentType), sizeof(currentType));
}

bool World::loadChunkFromFile(const glm::ivec3& pos, Chunk& chunk)
{
    const std::string fileName = "world_save/chunk_" + std::to_string(pos.x) + "_" + std::to_string(pos.y) + "_" + std::to_string(pos.z) + ".dat"; 

    if (!std::filesystem::exists(fileName)) return false; // we dont have any information about this chunk right now

    std::ifstream in(fileName, std::ios::binary);
    if (!in.is_open()) return false;

    Block* blockArray = &chunk.blocks[0][0][0];
    int totalBlocks = Chunk::WIDTH * Chunk::HEIGHT * Chunk::LENGTH;
    int blocksRead = 0;

    while (blocksRead < totalBlocks)
    {
        uint16_t runLength = 0;
        BlockType blockType = BlockType::Air;

        if (!in.read(reinterpret_cast<char*>(&runLength), sizeof(runLength))) break;
        if (!in.read(reinterpret_cast<char*>(&blockType), sizeof(blockType))) break;

        for (uint16_t i = 0; i < runLength; ++i)
        {
            if (blocksRead < totalBlocks)
            {
                blockArray[blocksRead].type = blockType;
                blocksRead++;
            }
        }
    }

    return true;
}

void World::generateCactuses(Chunk& chunk, int x, int z)
{
    if (x < 2 || x > Chunk::WIDTH - 3 || z < 2 || z > Chunk::LENGTH - 3) 
        return;

    int globalX = chunk.pos.x * Chunk::WIDTH + x;
    int globalZ = chunk.pos.z * Chunk::LENGTH + z;

    if (getBiomeAt(globalX, globalZ) == BiomeType::Desert)
    {
        int surfaceY = findSurfaceHight(chunk, x, z);

        if (surfaceY != 0 && chunk.blocks[x][surfaceY][z].type == BlockType::Sand)
        {
            // pseudo random
            float spawnChance = fnlGetNoise2D(&riverNoise, globalX * 50.f, globalZ * 50.f);
            
            if (spawnChance > 0.75f)
            {
                int cactusHeight = 2 + (int)((spawnChance - 0.75f) * 10.f) % 3;

                for (int h = 1; h <= cactusHeight; ++h)
                {
                    if (surfaceY + h < Chunk::HEIGHT) 
                    {
                        chunk.blocks[x][surfaceY + h][z].type = BlockType::Cactus;
                    }
                }
            }
        }
    }
}

void World::generateTrees(Chunk& chunk, int x, int z)
{
    if (x < 2 || x > Chunk::WIDTH - 3 || z < 2 || z > Chunk::LENGTH - 3) 
        return; // this thing is cool, really. I dont need to deal with 'ghost' blocks in chunks

    int globalX = chunk.pos.x * Chunk::WIDTH + x;
    int globalZ = chunk.pos.z * Chunk::LENGTH + z;

    if (getBiomeAt(globalX, globalZ) == BiomeType::Forest || getBiomeAt(globalX, globalZ) == BiomeType::Plains)
    {
        int surfaceY = findSurfaceHight(chunk, x, z);

        if (surfaceY != 0 && chunk.blocks[x][surfaceY][z].type == BlockType::Grass)
        {
            float spawnChance = fnlGetNoise2D(&riverNoise, globalX * 50.f, globalZ * 50.f);
            
            if (spawnChance > 0.75f)
            {
                int treeHeight = 4 + (int)((spawnChance - 0.75f) * 10.f) % 3;
                int trunkTopY = surfaceY + treeHeight;

                for (int ly = trunkTopY - 1; ly <= trunkTopY; ++ly)
                    for (int ox = -2; ox <= 2; ++ox)
                        for (int oz = -2; oz <= 2; ++oz)
                        {
                            if (std::abs(ox) == 2 && std::abs(oz) == 2) continue;
                            
                            if (ly < Chunk::HEIGHT)
                                chunk.blocks[x + ox][ly][z + oz].type = BlockType::Leaves;
                        }

                for (int ly = trunkTopY + 1; ly <= trunkTopY + 2; ++ly)
                    for (int ox = -1; ox <= 1; ++ox)
                        for (int oz = -1; oz <= 1; ++oz)
                        {
                            if (ly == trunkTopY + 2 && std::abs(ox) == 1 && std::abs(oz) == 1) continue;
                            
                            if (ly < Chunk::HEIGHT)
                                chunk.blocks[x + ox][ly][z + oz].type = BlockType::Leaves;
                        }


                for (int h = 1; h <= treeHeight; ++h)
                {
                    if (surfaceY + h < Chunk::HEIGHT) 
                    {
                        chunk.blocks[x][surfaceY + h][z].type = BlockType::Oak;
                    }
                }
            }
        }
    }
}

void World::generateFlowers(Chunk& chunk, int x, int z)
{
    int globalX = chunk.pos.x * Chunk::WIDTH + x;
    int globalZ = chunk.pos.z * Chunk::LENGTH + z;

    BiomeType biome = getBiomeAt(globalX, globalZ);
    if (biome == BiomeType::Desert) return;

    int surfaceY = findSurfaceHight(chunk, x, z);

    if (surfaceY > 0)
    {
        BlockType surfaceBlock = chunk.blocks[x][surfaceY][z].type;
        bool isValidSoil = (surfaceBlock == BlockType::Grass);
        
        if (isValidSoil && chunk.blocks[x][surfaceY + 1][z].type == BlockType::Air)
        {
            float plantNoise = fnlGetNoise2D(&riverNoise, globalX * 30.f, globalZ * 30.f);

            if (plantNoise > 0.6f) 
            {
                if (plantNoise > 0.75f)
                {
                    chunk.blocks[x][surfaceY + 1][z].type = BlockType::Flower;
                }
                else
                {
                    chunk.blocks[x][surfaceY + 1][z].type = BlockType::SmallGrass;
                }
            }
        }
    }
}

BlockType World::calculateBlockType(int globalX, int globalY, int globalZ)
{
    const uint8_t SEA_LEVEL = 22;
    const float RIVER_WIDTH = 0.4f;

    BiomeType biome = getBiomeAt(globalX, globalZ);

    float noiseVal = fnlGetNoise2D(&noise, static_cast<float>(globalX), static_cast<float>(globalZ));
    float noiseValWrapped = fnlGetNoise2D(&noise, static_cast<float>(globalX) + noiseVal, static_cast<float>(globalZ) + noiseVal);

    float baseHeight = 30.0f;
    switch (biome)
    {
        case BiomeType::Desert:  baseHeight = 26.0f + noiseValWrapped * 4.0f;  break;
        case BiomeType::Forest:  baseHeight = 28.0f + noiseValWrapped * 18.0f; break;
        case BiomeType::Plains:  baseHeight = 27.0f + noiseValWrapped * 12.0f; break;
        case BiomeType::Tundra:  baseHeight = 28.0f + noiseValWrapped * 16.0f; break;
    }

    float oceanVal = fnlGetNoise2D(&oceanNoise, globalX, globalZ);
    if (oceanVal < 0.1f)
    {
        float oceanDepthMask = std::abs(oceanVal);
        baseHeight = glm::mix(baseHeight, static_cast<float>(SEA_LEVEL - 8), oceanDepthMask);
    }

    float riverVal = std::abs(fnlGetNoise2D(&riverNoise, globalX, globalZ));

    if (riverVal < RIVER_WIDTH)
    {
        float riverMask = 1.f - (riverVal / RIVER_WIDTH);
        riverMask = riverMask * riverMask * (3.0f - 2.0f * riverMask);

        float targetRiverDepth = static_cast<float>(SEA_LEVEL - 3);

        if (baseHeight > targetRiverDepth)
        {
            baseHeight = glm::mix(baseHeight, targetRiverDepth, riverMask);
        }
    }

    int terrainHeight = static_cast<int>(baseHeight);

    if (globalY > terrainHeight)
    {
        if (globalY <= SEA_LEVEL)
            return BlockType::Water;
        
        return BlockType::Air;
    }

    else if (globalY == terrainHeight) 
    {
        if (terrainHeight < SEA_LEVEL - 1)
        {
            return BlockType::Sand; 
        }
        if (terrainHeight == SEA_LEVEL || terrainHeight == SEA_LEVEL - 1)
        {
            return (biome == BiomeType::Tundra) ? BlockType::SnowGrass : BlockType::Sand;
        }

        switch (biome)
        {
            case BiomeType::Desert: return BlockType::Sand;
            case BiomeType::Tundra: return BlockType::SnowGrass;
            default:                return BlockType::Grass;
        }
    } 
    else if (globalY > terrainHeight - 4) 
    {
        if (terrainHeight < SEA_LEVEL - 1) return BlockType::Sand;

        return (biome == BiomeType::Desert) ? BlockType::Sand : BlockType::Dirt;
    } 
    else 
    {
        return BlockType::Stone;
    }
}

BlockType World::getBlockAt(const glm::ivec3& globalPos) const
{
    glm::ivec3 chunkPos;
    chunkPos.x = globalPos.x >= 0 ? globalPos.x / Chunk::WIDTH  : (globalPos.x - Chunk::WIDTH + 1) / Chunk::WIDTH;
    chunkPos.y = globalPos.y >= 0 ? globalPos.y / Chunk::HEIGHT : (globalPos.y - Chunk::HEIGHT + 1) / Chunk::HEIGHT;
    chunkPos.z = globalPos.z >= 0 ? globalPos.z / Chunk::LENGTH : (globalPos.z - Chunk::LENGTH + 1) / Chunk::LENGTH;

    auto it = chunks.find(chunkPos);
    if (it == chunks.end())
        return BlockType::Air; // we at the end of map, so, there is only air

    int localX = ((globalPos.x % Chunk::WIDTH) + Chunk::WIDTH) % Chunk::WIDTH;
    int localY = ((globalPos.y % Chunk::HEIGHT) + Chunk::HEIGHT) % Chunk::HEIGHT;
    int localZ = ((globalPos.z % Chunk::LENGTH) + Chunk::LENGTH) % Chunk::LENGTH;

    return it->second->blocks[localX][localY][localZ].type;
}

BiomeType World::getBiomeAt(int globalX, int globalZ)
{
    float t = fnlGetNoise2D(&tempNoise, globalX, globalZ);
    float m = fnlGetNoise2D(&moistureNoise, globalX, globalZ);

    if (t > 0.2f)
    {
        if (m < -0.1f) return BiomeType::Desert;
        else return BiomeType::Forest;
    }
    else
    {   
        if (m < -0.1f) return BiomeType::Tundra;
        else return BiomeType::Plains;
    }
}

int World::findSurfaceHight(const Chunk& chunk, int x, int z)
{
    for (int y = Chunk::HEIGHT - 2; y > 0; --y)
    {
        if (chunk.blocks[x][y][z].type != BlockType::Air && chunk.blocks[x][y + 1][z].type == BlockType::Air)
            return y;
    }

    return 0;
}