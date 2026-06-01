#include <game/chunk.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

void Chunk::createChunk(Device& device)
{
    pos = glm::ivec3(0.f);

    for (int x = 0; x < WIDTH; ++x)
        for (int y = 0; y < HEIGHT; ++y)
            for (int z = 0; z < LENGTH; ++z)
            {
                if (y == 0 || y == HEIGHT-1 ||
                    x == 0 || x == WIDTH-1 ||
                    z == 0 || z == LENGTH-1)
                {
                    blocks[x][y][z].type = BlockType::Stone;
                }
                else
                {
                    blocks[x][y][z].type = BlockType::Air;
                }
            }
    createMesh(device);
    modelMatrix = glm::translate(glm::mat4(1.f), glm::vec3(pos.x * WIDTH, pos.y * HEIGHT, pos.z * LENGTH));
}

void Chunk::createMesh(Device& device)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // lamda function, we will need this
    auto addFace = [&](glm::vec3 pos, BlockFace face, BlockUV uv)
    {
        uint32_t start = vertices.size();

        float x0 = uv.topLeft.x;
        float y0 = uv.topLeft.y;
        float x1 = uv.bottomRight.x;
        float y1 = uv.bottomRight.y;

        switch (face)
        {
        case BlockFace::TOP:
            vertices.push_back({pos + glm::vec3(0,1,0), {1,1,1}, {x1, y0}});
            vertices.push_back({pos + glm::vec3(1,1,0), {1,1,1}, {x0, y0}});
            vertices.push_back({pos + glm::vec3(1,1,1), {1,1,1}, {x0, y1}});
            vertices.push_back({pos + glm::vec3(0,1,1), {1,1,1}, {x1, y1}});

            break;
        case BlockFace::BOTTOM:
            vertices.push_back({pos + glm::vec3(0,0,0), {1,1,1}, {x1, y0}});
            vertices.push_back({pos + glm::vec3(1,0,0), {1,1,1}, {x0, y0}});
            vertices.push_back({pos + glm::vec3(1,0,1), {1,1,1}, {x0, y1}});
            vertices.push_back({pos + glm::vec3(0,0,1), {1,1,1}, {x1, y1}});

            break;
        case BlockFace::FRONT:
            vertices.push_back({pos + glm::vec3(0,0,1), {1,1,1}, {x1, y0}});
            vertices.push_back({pos + glm::vec3(1,0,1), {1,1,1}, {x0, y0}});
            vertices.push_back({pos + glm::vec3(1,1,1), {1,1,1}, {x0, y1}});
            vertices.push_back({pos + glm::vec3(0,1,1), {1,1,1}, {x1, y1}});
            
            break;
        case BlockFace::BACK:
            vertices.push_back({pos + glm::vec3(0,0,0), {1,1,1}, {x1, y0}});
            vertices.push_back({pos + glm::vec3(1,0,0), {1,1,1}, {x0, y0}});
            vertices.push_back({pos + glm::vec3(1,1,0), {1,1,1}, {x0, y1}});
            vertices.push_back({pos + glm::vec3(0,1,0), {1,1,1}, {x1, y1}});
            
            break;
        case BlockFace::LEFT:
            vertices.push_back({pos + glm::vec3(0,0,0), {1,1,1}, {x1, y0}});
            vertices.push_back({pos + glm::vec3(0,0,1), {1,1,1}, {x0, y0}});
            vertices.push_back({pos + glm::vec3(0,1,1), {1,1,1}, {x0, y1}});
            vertices.push_back({pos + glm::vec3(0,1,0), {1,1,1}, {x1, y1}});

            break;
        case BlockFace::RIGHT:
            vertices.push_back({pos + glm::vec3(1,0,0), {1,1,1}, {x1, y0}});
            vertices.push_back({pos + glm::vec3(1,0,1), {1,1,1}, {x0, y0}});
            vertices.push_back({pos + glm::vec3(1,1,1), {1,1,1}, {x0, y1}});
            vertices.push_back({pos + glm::vec3(1,1,0), {1,1,1}, {x1, y1}});

            break;
        
        default:
            break;
        }

        indices.insert(indices.end(), {
            start + 0, start + 1, start + 2,
            start + 2, start + 3, start + 0
        });
    };

    for (int x = 0; x < WIDTH; ++x)
        for (int y = 0; y < HEIGHT; ++y)
            for (int z = 0; z < LENGTH; ++z)
            {
                BlockType currentType = blocks[x][y][z].type;
                if (currentType == BlockType::Air)
                    continue;

                glm::vec3 p(x, y, z);

                // for each face, we need check for neighbour
                // +x axis
                if (x == WIDTH - 1 || blocks[x + 1][y][z].type == BlockType::Air)
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::RIGHT);
                    addFace(p, BlockFace::RIGHT, uv);
                }
                
                // -x axis
                if (x == 0 || blocks[x-1][y][z].type == BlockType::Air)
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::LEFT);
                    addFace(p, BlockFace::LEFT, uv);
                }

                // +y axis
                if (y == HEIGHT - 1 || blocks[x][y+1][z].type == BlockType::Air)
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::TOP);
                    addFace(p, BlockFace::TOP, uv);
                }

                // -y axis
                if (y == 0 || blocks[x][y-1][z].type == BlockType::Air)
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::BOTTOM);
                    addFace(p, BlockFace::BOTTOM, uv);
                }

                // +z axis
                if (z == LENGTH - 1 || blocks[x][y][z+1].type == BlockType::Air)
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::FRONT);
                    addFace(p, BlockFace::FRONT, uv);
                }

                // -z axis
                if (z == 0 || blocks[x][y][z-1].type == BlockType::Air)
                {
                    BlockUV uv = getBlockTextureUV(currentType, BlockFace::BACK);
                    addFace(p, BlockFace::BACK, uv);
                }
            } 

    mesh.create(device, vertices, indices);

    std::cout << "Vertices: " << vertices.size() << '\n';
    std::cout << "Indices: " << indices.size() << '\n';
}

void Chunk::cleanup(VkDevice device)
{
    mesh.cleanup(device);
}

const Mesh& Chunk::getMesh() const
{
    return mesh;
}

glm::mat4 Chunk::getModelMatrix() const
{
    return modelMatrix;
}