#include <game/player.hpp>

Player::Player(glm::vec3 pos, float speed, float fov, uint32_t windowWidth, uint32_t windowHeight) 
    : position(pos), speed(speed), fov(fov)
{
    playerCamera = std::make_unique<Camera>(position, fov, windowWidth, windowHeight);
}

Player::~Player() { /* destructor for Camera will call automaticly */ }

void Player::update(GLFWwindow* window, float deltaTime)
{
    input(window, deltaTime);
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