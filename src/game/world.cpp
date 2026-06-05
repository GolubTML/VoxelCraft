#define FNL_IMPL
#include <game/world.hpp>
#include <game/chunk.hpp>
#include <core/device.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>

World::World(int seed) : worldSeed(seed)
{
    noise = fnlCreateState();
    noise.seed = worldSeed;
    noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    noise.fractal_type = FNL_FRACTAL_FBM;
    noise.octaves = 4;
    noise.frequency = 0.005f;

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

    for (int x = 0; x < Chunk::WIDTH; ++x)
        for (int y = 0; y < Chunk::HEIGHT; ++y)
            for (int z = 0; z < Chunk::LENGTH; ++z)
            {
                BlockType currentType = chunk.blocks[x][y][z].type;
                if (currentType == BlockType::Air)
                    continue;

                glm::vec3 localPos(x, y, z);
                glm::ivec3 globalPos(chunk.pos.x * Chunk::WIDTH + x, chunk.pos.y * Chunk::HEIGHT + y, chunk.pos.z * Chunk::LENGTH + z);

                // for each face, we need check for neighbour
                // +x axis
                if (getBlockAt(globalPos + glm::ivec3(1, 0, 0)) == BlockType::Air)
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::RIGHT);
                    glm::vec3 color = getBlockFaceColor(currentType, BlockFace::RIGHT);
                    addFace(localPos, color, BlockFace::RIGHT, uv);
                }
                
                // -x axis
                if (getBlockAt(globalPos + glm::ivec3(-1, 0, 0)) == BlockType::Air)
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::LEFT);
                    glm::vec3 color = getBlockFaceColor(currentType, BlockFace::LEFT);
                    addFace(localPos, color, BlockFace::LEFT, uv);
                }

                // +y axis
                if (getBlockAt(globalPos + glm::ivec3(0, 1, 0)) == BlockType::Air)
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::TOP);
                    glm::vec3 color = getBlockFaceColor(currentType, BlockFace::TOP);
                    addFace(localPos, color, BlockFace::TOP, uv);
                }

                // -y axis
                if (getBlockAt(globalPos + glm::ivec3(0, -1, 0)) == BlockType::Air)
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::BOTTOM);
                    glm::vec3 color = getBlockFaceColor(currentType, BlockFace::BOTTOM);
                    addFace(localPos, color, BlockFace::BOTTOM, uv);
                }

                // +z axis
                if (getBlockAt(globalPos + glm::ivec3(0, 0, 1)) == BlockType::Air)
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::FRONT);
                    glm::vec3 color = getBlockFaceColor(currentType, BlockFace::FRONT);
                    addFace(localPos, color, BlockFace::FRONT, uv);
                }

                // -z axis
                if (getBlockAt(globalPos + glm::ivec3(0, 0, -1)) == BlockType::Air)
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::BACK);
                    glm::vec3 color = getBlockFaceColor(currentType, BlockFace::BACK);
                    addFace(localPos, color, BlockFace::BACK, uv);
                }
            } 

    chunk.modelMatrix = glm::translate(glm::mat4(1.0f), 
        glm::vec3(chunk.pos.x * Chunk::WIDTH, 
            chunk.pos.y * Chunk::HEIGHT, 
            chunk.pos.z * Chunk::LENGTH));

    return {vertices, indices};
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
                                    it->second->mesh.create(*devicePtr, vertices, indices);
        
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

                        it->second->mesh.cleanup(devicePtr->getDevice());
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

    out.write(reinterpret_cast<const char*>(chunk.blocks), sizeof(chunk.blocks));
}

bool World::loadChunkFromFile(const glm::ivec3& pos, Chunk& chunk)
{
    const std::string fileName = "world_save/chunk_" + std::to_string(pos.x) + "_" + std::to_string(pos.y) + "_" + std::to_string(pos.z) + ".dat"; 

    if (!std::filesystem::exists(fileName)) return false; // we dont have any information about this chunk right now

    std::ifstream in(fileName, std::ios::binary);
    if (!in.is_open()) return false;

    in.read(reinterpret_cast<char*>(chunk.blocks), sizeof(chunk.blocks));
    return true;
}

BlockType World::calculateBlockType(int globalX, int globalY, int globalZ)
{
    float noiseVal = fnlGetNoise2D(&noise, static_cast<float>(globalX), static_cast<float>(globalZ));
    
    float noiseValWrapped = fnlGetNoise2D(&noise, static_cast<float>(globalX) + noiseVal, static_cast<float>(globalZ) + noiseVal);

    // 30, like base level of terrain
    // and 25 is level of mountains
    int terrainHeight = 30 + static_cast<int>((noiseValWrapped + 1.0f) * 0.5f * 25.0f);

    if (globalY > terrainHeight) 
    {
        return BlockType::Air;
    } 
    else if (globalY == terrainHeight) 
    {
        return BlockType::Grass;
    } 
    else if (globalY > terrainHeight - 4) 
    {
        return BlockType::Dirt;
    } 
    else 
    {
        return BlockType::Stone;
    }
}