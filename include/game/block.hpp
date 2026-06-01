#pragma once

enum BlockType
{
    Air, 
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
