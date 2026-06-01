#pragma once

#include <glm/glm.hpp>

enum BlockType
{
    Air, 
    UNKNOW,
    Dirt,
    Grass,
    Stone,
};

enum BlockFace
{
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
    FRONT,
    BACK
};

struct Block
{
    BlockType type = BlockType::Air;
};

// we need to calculate for texture in atlas, so, let's make new structure
struct BlockUV
{
    glm::vec2 topLeft;
    glm::vec2 bottomRight;
};

BlockUV calculateUV(int gridX, int gridY);
BlockUV getBlockTextureUV(BlockType type, BlockFace face);
glm::vec3 getBlockFaceColor(BlockType type, BlockFace face); // helper function. Some textures in atlas are gray, it means, thath is a mask for real texture