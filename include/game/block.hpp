#pragma once

#include <glm/glm.hpp>

enum BlockType : uint8_t
{
    Air, 
    UNKNOW,
    Dirt,
    Grass,
    SnowGrass,
    Stone,
    Sand,
    Water, // now, it's just a solid block. Need to make it transperent
    Cactus,
    Oak,
    Leaves,

    Flower,
    SmallGrass,

    Glass,
    Bedrock
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
    // bool transparent = false;
};

bool isBlockTransparent(BlockType type);
bool isBlockCrossed(BlockType type); // this is also bad, but this is the only way to make it

// we need to calculate for texture in atlas, so, let's make new structure
struct BlockUV
{
    glm::vec2 topLeft;
    glm::vec2 bottomRight;
};

BlockUV calculateUV(int gridX, int gridY);
BlockUV getBlockTextureUV(BlockType type, BlockFace face);
uint32_t getBlockFaceColor(BlockType type, BlockFace face); // helper function. Some textures in atlas are gray, it means, thath is a mask for real texture