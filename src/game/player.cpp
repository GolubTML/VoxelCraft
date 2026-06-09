#include <game/player.hpp>
#include <game/world.hpp>

Player::Player(glm::vec3 pos, float speed, float fov, uint32_t windowWidth, uint32_t windowHeight) 
    : position(pos), speed(speed), fov(fov)
{
    playerCamera = std::make_unique<Camera>(position, fov, windowWidth, windowHeight);
    currentBlock = BlockType::Stone;
}

Player::~Player() { /* destructor for Camera will call automaticly */ }

void Player::update(GLFWwindow* window, float deltaTime, World& world)
{
    input(window, deltaTime);

    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
    {
        static bool leftPressed = false;
        static bool rightPressed = false;
        static bool middlePressed = false;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !leftPressed)
        {
            RaycastResult result = raycast(world, playerCamera->pos, playerCamera->front, 5.f);

            if (result.hit)
                world.setBlock(result.blockPos, BlockType::Air);

            leftPressed = true;
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) leftPressed = false;
        
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && !rightPressed)
        {
            RaycastResult result = raycast(world, playerCamera->pos, playerCamera->front, 5.f);

            if (result.hit)
            {
                glm::vec3 placePos = result.blockPos + result.normal;
                world.setBlock(placePos, currentBlock);
            }

            rightPressed = true;
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) rightPressed = false;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS && !middlePressed)
        {
            RaycastResult result = raycast(world, playerCamera->pos, playerCamera->front, 5.f);

            if (result.hit)
            {
                currentBlock = world.getBlock(result.blockPos);
            }

            middlePressed = true;
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE) middlePressed = false;
    }    
}

void Player::handleMouse(GLFWwindow* window, double xpos, double ypos)
{
    playerCamera->mouse_callback(window, xpos, ypos);
}

Camera& Player::getPlayerCamera() const
{
    return *playerCamera;
}

glm::vec3& Player::getPlayerPosition()
{
    return position;
}

void Player::input(GLFWwindow* window, float deltaTime)
{
    float velocity = speed * deltaTime;

    glm::vec3 right = glm::normalize(glm::cross(playerCamera->front, playerCamera->up));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        position += playerCamera->front * velocity;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        position -= playerCamera->front * velocity;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        position -= right * velocity;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        position += right * velocity;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        position.y += velocity;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        position.y -= velocity;

    playerCamera->pos = position;
}

RaycastResult Player::raycast(const World& world, glm::vec3& origin, glm::vec3& dir, float dist)
{
    RaycastResult result;   
    glm::vec3 direction = glm::normalize(dir);
    glm::ivec3 blockPos = glm::floor(origin);

    glm::vec3 deltaDist(
        direction.x == 0.0f ? 1e30f : std::abs(1.0f / direction.x),
        direction.y == 0.0f ? 1e30f : std::abs(1.0f / direction.y),
        direction.z == 0.0f ? 1e30f : std::abs(1.0f / direction.z)
    );

    glm::ivec3 step;
    glm::vec3 sideDist;

    if (direction.x < 0.f) 
    {
        step.x = -1;
        sideDist.x = (origin.x - blockPos.x) * deltaDist.x;
    }
    else 
    {
        step.x = 1;
        sideDist.x = (blockPos.x + 1.f - origin.x) * deltaDist.x;
    }

    if (direction.y < 0.f)
    {
        step.y = -1;
        sideDist.y = (origin.y - blockPos.y) * deltaDist.y;
    }
    else 
    {
        step.y = 1;
        sideDist.y = (blockPos.y + 1.f - origin.y) * deltaDist.y;
    }

    if (direction.z < 0.f)
    {
        step.z = -1;
        sideDist.z = (origin.z - blockPos.z) * deltaDist.z;
    }
    else 
    {
        step.z = 1;
        sideDist.z = (blockPos.z + 1.f - origin.z) * deltaDist.z;
    }

    float distance = 0.f;
    glm::ivec3 lastNormal(0);

    while (distance < dist)
    {
        if (world.getBlock(blockPos) != BlockType::Air /*|| world.getBlock(blockPos) != BlockType::Water */)
        {
            result.hit = true;
            result.blockPos = blockPos;
            result.normal = lastNormal;
            return result;
        }

        if (sideDist.x < sideDist.y)
        {
            if (sideDist.x < sideDist.z) 
            {
                distance = sideDist.x;
                sideDist.x += deltaDist.x;
                blockPos.x += step.x;
                lastNormal = glm::ivec3(-step.x, 0, 0);
            } 
            else 
            {
                distance = sideDist.z;
                sideDist.z += deltaDist.z;
                blockPos.z += step.z;
                lastNormal = glm::ivec3(0, 0, -step.z);
            }
        } 
        else 
        {
            if (sideDist.y < sideDist.z) 
            {
                distance = sideDist.y;
                sideDist.y += deltaDist.y;
                blockPos.y += step.y;
                lastNormal = glm::ivec3(0, -step.y, 0);
            } 
            else 
            {
                distance = sideDist.z;
                sideDist.z += deltaDist.z;
                blockPos.z += step.z;
                lastNormal = glm::ivec3(0, 0, -step.z);
            }
        }
    }

    return result;
}