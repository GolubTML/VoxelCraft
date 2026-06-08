#include <game/block.hpp>
#include <renderer/mesh.hpp>

bool isBlockTransparent(BlockType type)
{
    switch (type)
    {
    case Air:
    case Water:
    case Leaves:
    case Flower:
    case SmallGrass:
    case Glass:
        return true;
    default:
        return false;
    }
}

bool isBlockCrossed(BlockType type)
{
    switch (type)
    {
    case Flower:
    case SmallGrass:
        return true;
    default:
        return false;
    }
}

BlockUV calculateUV(int gridX, int gridY)
{
    // it's hard coded, but for now it's okey
    // 32.f because width of texture if 512, so 512/16 = 32
    // 16.f the same as width, but for heigth
    float const texWidthBlocks = 32.0f;
    float const texHeightBlocks = 16.0f;

    BlockUV uv;
    uv.topLeft.x = static_cast<float>(gridX) / texWidthBlocks;
    uv.topLeft.y = static_cast<float>(gridY) / texHeightBlocks;
    
    uv.bottomRight.x = static_cast<float>(gridX + 1) / texWidthBlocks;
    uv.bottomRight.y = static_cast<float>(gridY + 1) / texHeightBlocks;
    
    return uv;
}

BlockUV getBlockTextureUV(BlockType type, BlockFace face)
{
    switch (type)
    {
    case BlockType::Dirt:
    {
        return calculateUV(8, 11); 
    }
    
    case BlockType::Stone:
    {
        return calculateUV(19, 5);
    }
    
    case BlockType::Sand:
    {
        return calculateUV(18, 5);
    }

    case BlockType::Leaves:
    {
        return calculateUV(1, 2);
    }
    
    case BlockType::Glass:
    {
        return calculateUV(15, 12);
    }

    case BlockType::Bedrock:
    {
        return calculateUV(4, 12);
    }

    case BlockType::Grass:
    {
        if (face == BlockFace::TOP)
            return calculateUV(3, 5);

        if (face == BlockFace::BOTTOM)
            return calculateUV(8, 11); 

        return calculateUV(0, 5);
    }
    case BlockType::SnowGrass:
    {
        if (face == BlockFace::TOP)
            return calculateUV(3, 5);

        if (face == BlockFace::BOTTOM)
            return calculateUV(8, 11); 

        return calculateUV(2, 5);
    }
    case BlockType::Cactus:
    {
        if (face == BlockFace::TOP)
            return calculateUV(6, 13);

        if (face == BlockFace::BOTTOM)
            return calculateUV(6, 15); 

        return calculateUV(6, 14);
    }
    
    case BlockType::Oak:
    {
        if (face == BlockFace::TOP || face == BlockFace::BOTTOM)
            return calculateUV(14, 2);

        return calculateUV(13, 2);
    }

    case BlockType::Water:
    {
        return calculateUV(2, 15);
    }

    case BlockType::Flower:
    {
        return calculateUV(14, 13);
    }
    
    case BlockType::SmallGrass:
    {
        return calculateUV(20, 8);
    }

    case BlockType::UNKNOW:
        return calculateUV(5, 1);

    default:
        return calculateUV(5, 1);
        break;
    }
}

uint32_t getBlockFaceColor(BlockType type, BlockFace face)
{
    // it's bad code, but in future i will make it better

    if (type == BlockType::Grass)
    {
        if (face == BlockFace::TOP)
        {
            return Vertex::packColor(109, 255, 61);
        }
    }
    
    if (type == BlockType::SnowGrass)
    {
        if (face == BlockFace::TOP)
        {
            return Vertex::packColor(255, 255, 255);
        }
    }

    if (type == BlockType::Leaves)
        return Vertex::packColor(109, 255, 61);

    if (type == BlockType::SmallGrass)
        return Vertex::packColor(64, 216, 64);

    return Vertex::packColor(255, 255, 255);
}