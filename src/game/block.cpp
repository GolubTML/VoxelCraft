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
    
    case BlockType::Sand:
    {
        return calculateUV(18, 5);
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
    case BlockType::Water:
    {
        return calculateUV(2, 15);
    }

    case BlockType::UNKNOW:
        return calculateUV(5, 1);

    default:
        return calculateUV(5, 1);
        break;
    }
}

glm::vec3 getBlockFaceColor(BlockType type, BlockFace face)
{
    // it's bad code, but in future i will make it better

    if (type == BlockType::Grass)
    {
        if (face == BlockFace::TOP)
        {
            return glm::vec3(0.43f, 1.f, 0.24f);
        }
    }
    
    if (type == BlockType::SnowGrass)
    {
        if (face == BlockFace::TOP)
        {
            return glm::vec3(1.5f, 1.5f, 1.5f);
        }
    }

    return glm::vec3(1.f);
}