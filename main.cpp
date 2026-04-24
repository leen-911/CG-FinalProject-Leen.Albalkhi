// 3D Lane Runner Game - FULLY FIXED VERSION

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include<vector>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#pragma warning(push)
#pragma warning(disable: 6262) 
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma warning(pop)            

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Game state
int   currentLane = 1;
float laneX[3] = { -1.5f, 0.0f, 1.5f };
float ballZ = -5.0f;
float ballY = 0.0f;
float ballSpeed = 5.0f;
float ballRotation = 0.0f;

float gameDuration = 60.0f;
float startTime = 0.0f;
int   score = 0;
bool  gameOver = false;
bool  hitObstacle = false;

// Jump variables
bool isJumping = false;
float verticalVelocity = 0.0f;
float gravity = -9.8f;
float jumpStrength = 6.0f;

// Coins
struct Coin {
    float x, y, z;
    bool collected;
};

const int NUM_COINS = 500;
Coin coins[NUM_COINS];

// Obstacles
struct Obstacle {
    float x, y, z;
    bool hit;
};
const int NUM_OBSTACLES = 80;
Obstacle obstacles[NUM_OBSTACLES];

// SHADERS

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"layout (location = 2) in vec2 aTexCoord;\n"
"layout (location = 3) in vec3 aNormal;\n"
"out vec3 vColor;\n"
"out vec2 vTexCoord;\n"
"out vec3 vNormal;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"    vColor = aColor;\n"
"    vTexCoord = aTexCoord;\n"
"    vNormal = mat3(transpose(inverse(model))) * aNormal;\n"
"}\n";

const char* fragmentShaderSource = "#version 330 core\n"
"in vec3 vColor;\n"
"in vec2 vTexCoord;\n"
"in vec3 vNormal;\n"
"out vec4 FragColor;\n"
"uniform sampler2D diffuseTex;\n"
"uniform float useTexture;\n"
"uniform float scroll;\n"
"uniform float useLighting;\n"

"void main(){\n"
"    vec3 lightDir = normalize(vec3(0.3, 1.0, 0.5));\n"
"    float diff = max(dot(normalize(vNormal), lightDir), 0.2);\n"
"    vec2 scrolledTex = vec2(vTexCoord.x, vTexCoord.y + scroll);\n"
"    vec4 texColor = texture(diffuseTex, scrolledTex);\n"
"    vec3 litColor = (useLighting > 0.5) ? (vColor * diff) : vColor;\n"
"    vec4 baseColor = vec4(litColor, 1.0);\n"

"    FragColor = mix(baseColor, texColor, useTexture);\n"
"}\n";

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void resetGame()
{
    // Reset all game variables
    currentLane = 1;
    ballZ = -5.0f;
    ballY = 0.0f;
    ballSpeed = 5.0f;
    ballRotation = 0.0f;
    score = 0;
    gameOver = false;
    hitObstacle = false;
    isJumping = false;
    verticalVelocity = 0.0f;
    startTime = glfwGetTime();  // Timer starts fresh

    // Reset all coins
    for (int i = 0; i < NUM_COINS; ++i) {
        int lane = rand() % 3;
        float z = -(8.0f + i * 1.8f);
        coins[i].x = laneX[lane];
        coins[i].y = 0.3f;
        coins[i].z = z;
        coins[i].collected = false;
    }

    // Reset all obstacles
    for (int i = 0; i < NUM_OBSTACLES; ++i) {
        int lane = rand() % 3;
        float z = -(15.0f + i * 5.0f);
        obstacles[i].x = laneX[lane];
        obstacles[i].y = 0.5f;
        obstacles[i].z = z;
        obstacles[i].hit = false;
    }

    cout << "\n========== GAME RESET ==========" << endl;
    cout << "New game started! Good luck!" << endl;
    cout << "===============================\n" << endl;
}

void processInput(GLFWwindow* window)
{
    static bool leftPressed = false;
    static bool rightPressed = false;
    static bool rPressed = false;

    // ESC to quit - ALWAYS works
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // R to reset game
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rPressed) {
        resetGame();
        rPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
        rPressed = false;
    }

    // Only allow lane movement if game is not over
    if (!gameOver) {
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && !leftPressed) {
            currentLane = std::max(0, currentLane - 1);
            leftPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_RELEASE) {
            leftPressed = false;
        }

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && !rightPressed) {
            currentLane = std::min(2, currentLane + 1);
            rightPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_RELEASE) {
            rightPressed = false;
        }

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && !isJumping) {
            isJumping = true;
            verticalVelocity = jumpStrength;
        }
    }
}

unsigned int loadTexture(const char* path)
{
    unsigned int textureID = 0;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data)
    {
        GLenum format = GL_RGB;
        if (nrChannels == 1)      format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
            GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        cout << "[OK] Loaded texture: " << path << endl;
    }
    else
    {
        cerr << "[FAIL] Could not load texture: " << path << endl;
        cerr << "      Continuing with default fallback color..." << endl;
        return 0;
    }

    return textureID;
}

// Create a simple cube for obstacles
void createCubeGeometry(std::vector<float>& vertices, std::vector<unsigned int>& indices)
{
    // Cube vertices with positions, normals, colors, and texcoords
    vertices = {
        // Front face  
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.2f, 0.2f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.2f, 0.2f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.2f, 0.2f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.2f, 0.2f,  0.0f, 1.0f,

        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 1.0f, 0.2f, 0.2f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 1.0f, 0.2f, 0.2f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 1.0f, 0.2f, 0.2f,  0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 1.0f, 0.2f, 0.2f,  1.0f, 1.0f,

        // Right face
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.2f, 0.2f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.2f, 0.2f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.2f, 0.2f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.2f, 0.2f,  0.0f, 1.0f,

         // Left face
         -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,  1.0f, 0.2f, 0.2f,  1.0f, 0.0f,
         -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f,  1.0f, 0.2f, 0.2f,  0.0f, 0.0f,
         -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f,  1.0f, 0.2f, 0.2f,  0.0f, 1.0f,
         -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f,  1.0f, 0.2f, 0.2f,  1.0f, 1.0f,

         // Top face
         -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.2f, 0.2f,  0.0f, 1.0f,
          0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.2f, 0.2f,  1.0f, 1.0f,
          0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.2f, 0.2f,  1.0f, 0.0f,
         -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.2f, 0.2f,  0.0f, 0.0f,

         // Bottom face
         -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 0.2f, 0.2f,  1.0f, 0.0f,
          0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 0.2f, 0.2f,  0.0f, 0.0f,
          0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 0.2f, 0.2f,  0.0f, 1.0f,
         -0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 0.2f, 0.2f,  1.0f, 1.0f,
    };

    indices = {
        // Front
        0, 1, 2, 2, 3, 0,
        // Back
        4, 6, 5, 4, 7, 6,
        // Right
        8, 9, 10, 10, 11, 8,
        // Left
        12, 14, 13, 12, 15, 14,
        // Top
        16, 17, 18, 18, 19, 16,
        // Bottom
        20, 21, 22, 22, 23, 20
    };
}

int main()
{
    srand((unsigned int)time(nullptr));

    cout << "=== 3D Lane Runner Starting ===" << endl;

    // GLFW init
    cout << "[INIT] Initializing GLFW..." << endl;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Game (Coins Collector 101)", NULL, NULL);
    if (window == NULL)
    {
        cerr << "[ERROR] Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    cout << "[OK] GLFW window created" << endl;

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // GLEW init
    cout << "[INIT] Initializing GLEW..." << endl;
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        cerr << "[ERROR] Failed to initialize GLEW" << endl;
        glfwTerminate();
        return -1;
    }
    cout << "[OK] GLEW initialized" << endl;

    glEnable(GL_DEPTH_TEST);

    // SHADER COMPILATION
    cout << "[INIT] Compiling shaders..." << endl;
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cerr << "[ERROR] Vertex shader compilation failed:\n" << infoLog << endl;
        glfwTerminate();
        return -1;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cerr << "[ERROR] Fragment shader compilation failed:\n" << infoLog << endl;
        glfwTerminate();
        return -1;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cerr << "[ERROR] Shader program linking failed:\n" << infoLog << endl;
        glfwTerminate();
        return -1;
    }
    cout << "[OK] Shaders compiled successfully" << endl;
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Uniform locations
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    int useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");
    int scrollLoc = glGetUniformLocation(shaderProgram, "scroll");
    int useLightingLoc = glGetUniformLocation(shaderProgram, "useLighting");

    // ROAD VAO/VBO/EBO Setup
    cout << "[INIT] Setting up road ..." << endl;
    float roadVertices[] = {
        -3.0f, 0.01f,  0.0f,       0.3f,0.3f,0.3f,   0.0f, 0.0f,
         3.0f, 0.01f,  0.0f,       0.3f,0.3f,0.3f,   3.0f, 0.0f,
         3.0f, 0.01f, -500.0f,      0.3f,0.3f,0.3f,   3.0f, 40.0f,
        -3.0f, 0.01f, -500.0f,      0.3f,0.3f,0.3f,   0.0f, 40.0f
    };
    unsigned int roadIndices[] = { 0,1,2, 0,2,3 };

    unsigned int roadVAO, roadVBO, roadEBO;
    glGenVertexArrays(1, &roadVAO);
    glGenBuffers(1, &roadVBO);
    glGenBuffers(1, &roadEBO);

    glBindVertexArray(roadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, roadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(roadVertices), roadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, roadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(roadIndices), roadIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    cout << "[OK] Road created" << endl;

    // === BALL (SPHERE) ===
    const int stacks = 20;
    const int slices = 20;
    std::vector<float> ballVertices;
    std::vector<unsigned int> ballIndices;

    for (int i = 0; i <= stacks; i++) {
        float V = (float)i / stacks;
        float phi = V * glm::pi<float>();

        for (int j = 0; j <= slices; j++) {
            float U = (float)j / slices;
            float theta = U * glm::two_pi<float>();

            float x = cos(theta) * sin(phi);
            float y = cos(phi);
            float z = sin(theta) * sin(phi);

            ballVertices.push_back(x * 0.5f);
            ballVertices.push_back(y * 0.5f);
            ballVertices.push_back(z * 0.5f);

            ballVertices.push_back(x);
            ballVertices.push_back(y);
            ballVertices.push_back(z);

            ballVertices.push_back(0.5f);
            ballVertices.push_back(0.0f);
            ballVertices.push_back(0.0f);
        }
    }

    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            int first = (i * (slices + 1)) + j;
            int second = first + slices + 1;

            ballIndices.push_back(first);
            ballIndices.push_back(second);
            ballIndices.push_back(first + 1);

            ballIndices.push_back(second);
            ballIndices.push_back(second + 1);
            ballIndices.push_back(first + 1);
        }
    }

    unsigned int ballVAO, ballVBO, ballEBO;
    glGenVertexArrays(1, &ballVAO);
    glGenBuffers(1, &ballVBO);
    glGenBuffers(1, &ballEBO);

    glBindVertexArray(ballVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ballVBO);
    glBufferData(GL_ARRAY_BUFFER, ballVertices.size() * sizeof(float), ballVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ballEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ballIndices.size() * sizeof(unsigned int), ballIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    cout << "[OK] Ball created" << endl;

    // COIN - Simple circle
    cout << "[INIT] Creating coin geometry..." << endl;
    const int segments = 40;
    vector<float> coinVertices;

    coinVertices.push_back(0.0f);    coinVertices.push_back(0.0f);    coinVertices.push_back(0.0f);
    coinVertices.push_back(1.0f);    coinVertices.push_back(0.84f);   coinVertices.push_back(0.0f);
    coinVertices.push_back(0.5f);    coinVertices.push_back(0.5f);

    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159265f * i / segments;
        float x = 0.3f * cosf(angle);
        float y = 0.3f * sinf(angle);

        coinVertices.push_back(x);
        coinVertices.push_back(y);
        coinVertices.push_back(0.0f);
        coinVertices.push_back(1.0f);    coinVertices.push_back(0.84f);   coinVertices.push_back(0.0f);
        coinVertices.push_back((cosf(angle) + 1.0f) / 2.0f);
        coinVertices.push_back((sinf(angle) + 1.0f) / 2.0f);
    }

    unsigned int coinVAO, coinVBO;
    glGenVertexArrays(1, &coinVAO);
    glGenBuffers(1, &coinVBO);

    glBindVertexArray(coinVAO);
    glBindBuffer(GL_ARRAY_BUFFER, coinVBO);
    glBufferData(GL_ARRAY_BUFFER, coinVertices.size() * sizeof(float), coinVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    int coinVertexCount = segments + 2;
    cout << "[OK] Coin created (" << coinVertexCount << " vertices)" << endl;

    // === OBSTACLE (CUBE) ===
    cout << "[INIT] Creating obstacle ..." << endl;
    std::vector<float> obstacleVertices;
    std::vector<unsigned int> obstacleIndices;
    createCubeGeometry(obstacleVertices, obstacleIndices);

    unsigned int obstacleVAO, obstacleVBO, obstacleEBO;
    glGenVertexArrays(1, &obstacleVAO);
    glGenBuffers(1, &obstacleVBO);
    glGenBuffers(1, &obstacleEBO);

    glBindVertexArray(obstacleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, obstacleVBO);
    glBufferData(GL_ARRAY_BUFFER, obstacleVertices.size() * sizeof(float), obstacleVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obstacleEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, obstacleIndices.size() * sizeof(unsigned int), obstacleIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    cout << "[OK] Obstacle created (" << obstacleIndices.size() << " indices)" << endl;

    // SKY
    cout << "[INIT] Creating sky..." << endl;
    float skySize = 1000.0f;
    float skyVertices[] = {
        // FRONT FACE
        -skySize, -skySize, -skySize,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,
         skySize, -skySize, -skySize,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,
         skySize,  skySize, -skySize,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,
        -skySize,  skySize, -skySize,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,

        // BACK FACE
        -skySize, -skySize,  skySize,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,
         skySize, -skySize,  skySize,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,
         skySize,  skySize,  skySize,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,
        -skySize,  skySize,  skySize,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,

        // TOP FACE
        -skySize,  skySize, -skySize,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,
         skySize,  skySize, -skySize,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,
         skySize,  skySize,  skySize,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,
        -skySize,  skySize,  skySize,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,

        // BOTTOM FACE
        -skySize, -skySize, -skySize,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,
         skySize, -skySize, -skySize,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,
         skySize, -skySize,  skySize,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,
        -skySize, -skySize,  skySize,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,

        // LEFT FACE
        -skySize, -skySize, -skySize,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,
        -skySize, -skySize,  skySize,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,
        -skySize,  skySize,  skySize,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,
        -skySize,  skySize, -skySize,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,

        // RIGHT FACE
         skySize, -skySize, -skySize,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,
         skySize, -skySize,  skySize,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,
         skySize,  skySize,  skySize,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,
         skySize,  skySize, -skySize,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,
    };

    unsigned int skyIndices[] = {
        0, 1, 2, 2, 3, 0,
        4, 6, 5, 4, 7, 6,
        8, 10, 9, 8, 11, 10,
        12, 13, 14, 14, 15, 12,
        16, 18, 17, 16, 19, 18,
        20, 21, 22, 22, 23, 20
    };

    unsigned int skyVAO, skyVBO, skyEBO;
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);
    glGenBuffers(1, &skyEBO);

    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyVertices), skyVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyIndices), skyIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    cout << "[OK] Sky created" << endl;

    // TEXTURES
    cout << "[INIT] Loading textures..." << endl;
    unsigned int roadTex = loadTexture("asphalt-road (1).jpg");
    unsigned int coinTex = loadTexture("gold-coin.png");
    unsigned int skyTex = loadTexture("sky.jpg");
    cout << "[OK] Texture loading complete" << endl;

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "diffuseTex"), 0);

    // Initialize coins
    cout << "[INIT] Spawning " << NUM_COINS << " coins..." << endl;
    for (int i = 0; i < NUM_COINS; ++i)
    {
        int lane = rand() % 3;
        float z = -(8.0f + i * 1.8f);
        coins[i].x = laneX[lane];
        coins[i].y = 0.3f;
        coins[i].z = z;
        coins[i].collected = false;
    }

    // Initialize obstacles
    cout << "[INIT] Spawning " << NUM_OBSTACLES << " obstacles..." << endl;
    for (int i = 0; i < NUM_OBSTACLES; ++i) {
        int lane = rand() % 3;
        float z = -(15.0f + i * 5.0f);
        obstacles[i].x = laneX[lane];
        obstacles[i].y = 0.5f;
        obstacles[i].z = z;
        obstacles[i].hit = false;
    }

    cout << "[OK] Game initialized successfully!" << endl;
    cout << "\n=== GAME START ===" << endl;
    cout << "Controls:" << endl;
    cout << "  LEFT/RIGHT arrows - Move between lanes" << endl;
    cout << "  UP arrow - Jump over obstacles" << endl;
    cout << "  R - Reset game" << endl;
    cout << "  ESC - Quit game" << endl;
    cout << "Objective: Collect coins, avoid obstacles!" << endl;
    cout << "Time limit: 60 seconds" << endl;
    cout << "===================\n" << endl;

    startTime = glfwGetTime();
    float lastFrame = startTime;

    int frameCount = 0;
    float fpsTimer = 0.0f;

    // MAIN LOOP
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        frameCount++;
        fpsTimer += deltaTime;

        // Always process input FIRST (ESC and R always work)
        processInput(window);

        // Calculate elapsed and remaining AFTER processing reset
        float elapsed = currentFrame - startTime;
        float remaining = gameDuration - elapsed;

        // Freeze timer at 0 when game is over (for ANY reason)
        if (gameOver) {
            remaining = 0.0f;
        }

        // Update window title with timer and score
        char title[128];
        snprintf(title, sizeof(title), "3D Game (Coins Collector 101) - Time: %.1f  |  Score: %d", remaining, score);
        glfwSetWindowTitle(window, title);

        // Handle jumping physics
        if (isJumping) {
            verticalVelocity += gravity * deltaTime;
            ballY += verticalVelocity * deltaTime;

            if (ballY <= 0.0f) {
                ballY = 0.0f;
                isJumping = false;
                verticalVelocity = 0.0f;
            }
        }

        // Time check
        if (remaining <= 0.0f && !gameOver) {
            gameOver = true;
            ballSpeed = 0.0f;
            cout << "\n========== TIME'S UP! ==========" << endl;
            cout << "Final Score: " << score << " coins" << endl;
            cout << "Press R to try again or ESC to quit" << endl;
            cout << "===============================\n" << endl;
        }

        // Obstacle collision check
        if (hitObstacle && !gameOver) {
            gameOver = true;
            ballSpeed = 0.0f;
            cout << "\n========== GAME OVER! ==========" << endl;
            cout << "You hit an obstacle!" << endl;
            cout << "Final Score: " << score << " coins" << endl;
            cout << "Press R to try again or ESC to quit" << endl;
            cout << "================================\n" << endl;
        }

        // Game physics (only if not game over)
        if (!gameOver) {
            float progressSpeed = ballSpeed * (1.0f + elapsed / gameDuration * 0.5f);
            ballZ -= progressSpeed * deltaTime;
            ballRotation += progressSpeed * deltaTime * 4.0f;
        }

        glm::vec3 carPos(laneX[currentLane], ballY, ballZ);

        // Collision detection with coins
        for (int i = 0; i < NUM_COINS; ++i) {
            if (coins[i].collected) continue;
            glm::vec3 coinPos(coins[i].x, coins[i].y, coins[i].z);
            float dist = glm::length(coinPos - carPos);
            if (dist < 0.5f) {
                coins[i].collected = true;
                score++;
            }
        }

        // Collision detection with obstacles
        for (int i = 0; i < NUM_OBSTACLES; ++i) {
            if (obstacles[i].hit) continue;
            glm::vec3 obsPos(obstacles[i].x, obstacles[i].y, obstacles[i].z);
            float dist = glm::length(obsPos - carPos);

            // If jumping, ignore obstacle collision
            if (isJumping) continue;

            if (dist < 0.7f) {
                obstacles[i].hit = true;
                hitObstacle = true;
            }
        }

        // Render
        glClearColor(0.4f, 0.7f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::vec3 cameraPos = glm::vec3(0.0f, 3.0f, ballZ + 7.0f);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.5f, ballZ - 5.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0, 1, 0));
        glm::mat4 projection = glm::perspective(glm::radians(40.0f),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 model;

        // Draw sky
        if (skyTex != 0) {
            glBindTexture(GL_TEXTURE_2D, skyTex);
            glUniform1f(useTextureLoc, 1.0f);
        }
        else {
            glUniform1f(useTextureLoc, 0.0f);
        }
        glUniform1f(useLightingLoc, 0.0f);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(skyVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // Draw road
        if (roadTex != 0) {
            glBindTexture(GL_TEXTURE_2D, roadTex);
            glUniform1f(useTextureLoc, 1.0f);
        }
        else {
            glUniform1f(useTextureLoc, 0.0f);
        }
        float scroll = fmod(currentFrame * 0.2f, 1.0f);
        glUniform1f(scrollLoc, scroll);
        model = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(roadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Draw ball
        glUniform1f(useLightingLoc, 1.0f);
        glUniform1f(useTextureLoc, 0.0f);

        model = glm::mat4(1.0f);
        model = glm::translate(model, carPos + glm::vec3(0.0f, 0.5f, 0.0f));
        model = glm::rotate(model, ballRotation, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(ballVAO);
        glDrawElements(GL_TRIANGLES, ballIndices.size(), GL_UNSIGNED_INT, 0);

        // Draw coins
        glUniform1f(useLightingLoc, 0.0f);
        glBindTexture(GL_TEXTURE_2D, coinTex);
        glUniform1f(useTextureLoc, 1.0f);
        glUniform1f(scrollLoc, 0.0f);
        glBindVertexArray(coinVAO);
        const float cullDistance = 30.0f;

        for (int i = 0; i < NUM_COINS; ++i) {
            if (coins[i].collected) continue;
            if (coins[i].z < ballZ - cullDistance) continue;

            glm::vec3 coinPos(coins[i].x, coins[i].y, coins[i].z);
            model = glm::mat4(1.0f);
            model = glm::translate(model, coinPos);
            model = glm::rotate(model, currentFrame * 2.0f, glm::vec3(0, 1, 0));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLE_FAN, 0, coinVertexCount);
        }

        // Draw obstacles
        glUniform1f(useLightingLoc, 1.0f);
        glUniform1f(useTextureLoc, 0.0f);
        glBindVertexArray(obstacleVAO);

        for (int i = 0; i < NUM_OBSTACLES; ++i) {
            if (obstacles[i].z < ballZ - cullDistance) continue;

            glm::vec3 obsPos(obstacles[i].x, obstacles[i].y, obstacles[i].z);
            model = glm::mat4(1.0f);
            model = glm::translate(model, obsPos);
            model = glm::scale(model, glm::vec3(0.6f, 0.8f, 0.6f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, obstacleIndices.size(), GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cout << "\n ====== Final Score: " << score << " coins ====== " << endl;
    cout << "\n *** Thanks for playing! ***\n" << endl;

    // Cleanup
    glDeleteBuffers(1, &roadVBO);
    glDeleteBuffers(1, &roadEBO);
    glDeleteVertexArrays(1, &roadVAO);

    glDeleteBuffers(1, &ballVBO);
    glDeleteBuffers(1, &ballEBO);
    glDeleteVertexArrays(1, &ballVAO);

    glDeleteBuffers(1, &coinVBO);
    glDeleteVertexArrays(1, &coinVAO);

    glDeleteBuffers(1, &obstacleVBO);
    glDeleteBuffers(1, &obstacleEBO);
    glDeleteVertexArrays(1, &obstacleVAO);

    glDeleteBuffers(1, &skyVBO);
    glDeleteBuffers(1, &skyEBO);
    glDeleteVertexArrays(1, &skyVAO);

    glDeleteProgram(shaderProgram);

    glfwTerminate();
    cout << "Game closed successfully." << endl;

    return 0;
}