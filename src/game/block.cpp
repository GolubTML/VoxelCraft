#include <game/block.hpp>

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

    case BlockType::Grass:
    {
        if (face == BlockFace::TOP)
            return calculateUV(3, 5);

        if (face == BlockFace::BOTTOM)
            return calculateUV(8, 11); 

        return calculateUV(0, 5);
    }

    case BlockType::UNKNOW:
        return calculateUV(5, 1);

    default:
        return calculateUV(5, 1);
        break;
    }
}