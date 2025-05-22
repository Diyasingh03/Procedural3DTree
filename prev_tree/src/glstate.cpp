#define NOMINMAX
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <string>
#include "glstate.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include "util.hpp"
#include "mesh.hpp"
#include <random>
#include <algorithm>
#include <GL/freeglut.h>
// Tree structure definitions
struct TreeParameters {
    float branchThickness = 2.0f;
    float treeHeight = 5.0f;
    float branchAngle = 30.0f;
    int maxDepth = 6;
    float branchProbability = 0.8f;
    float branchLengthFactor = 0.7f;
    float branchRadiusFactor = 0.6f;
    glm::vec3 leafColor = glm::vec3(1.0f, 0.5f, 0.5f); // Pink cherry blossom
    glm::vec3 barkColor = glm::vec3(0.6f, 0.3f, 0.2f); // Brown bark
};

struct TreeBranch {
    glm::vec3 start;
    glm::vec3 end;
    float radius;
    int depth;
};

// Constructor
GLState::GLState() :
    shadingMode(SHADINGMODE_PHONG),
    normalMapMode(NORMAL_MAPPING_ON),
    shadowMapMode(SHADOW_MAPPING_ON),
    width(1), height(1),
    fovy(45.0f),
    camCoords(0.0f, 1.0f, 4.5f),
    camRotating(false),
    shader(0),
    depthShader(0),
    treeShader(0),
    modelMatLoc(0),
    modelMatDepthLoc(0),
    lightSpaceMatLoc(0),
    lightSpaceMatDepthLoc(0),
    objTypeLoc(0),
    viewProjMatLoc(0),
    shadingModeLoc(0),
    normalMapModeLoc(0),
    shadowMapModeLoc(0),
    camPosLoc(0),
    floorColorLoc(0),
    floorAmbStrLoc(0),
    floorDiffStrLoc(0),
    floorSpecStrLoc(0),
    floorSpecExpLoc(0),
    cubeColorLoc(0),
    cubeAmbStrLoc(0),
    cubeDiffStrLoc(0),
    cubeSpecStrLoc(0),
    cubeSpecExpLoc(0),
    // Tree-related initialization
    treeVAO(0),
    treeBranchVBO(0),
    treeLeafVBO(0),
    treeIBO(0),
    uiEnabled(true),
    selectedParameter(0),
    showUIControls(true)
{
    // Initialize tree parameters with default values
    treeParams.branchThickness = 2.0f;
    treeParams.treeHeight = 5.0f;
    treeParams.branchAngle = 30.0f;
    treeParams.maxDepth = 6;
    treeParams.branchProbability = 0.8f;
    treeParams.branchLengthFactor = 0.7f;
    treeParams.branchRadiusFactor = 0.6f;
    treeParams.leafColor = glm::vec3(1.0f, 0.5f, 0.5f); // Pink cherry blossom
    treeParams.barkColor = glm::vec3(0.6f, 0.3f, 0.2f); // Brown bark
}

// Destructor
GLState::~GLState() {
    // Release OpenGL resources
    if (shader)	glDeleteProgram(shader);
    if (depthShader) glDeleteProgram(depthShader);
    if (treeShader) glDeleteProgram(treeShader);

    // Delete tree rendering resources
    if (treeVAO) glDeleteVertexArrays(1, &treeVAO);
    if (treeBranchVBO) glDeleteBuffers(1, &treeBranchVBO);
    if (treeLeafVBO) glDeleteBuffers(1, &treeLeafVBO);
    if (treeIBO) glDeleteBuffers(1, &treeIBO);
}

// Tree generation methods
void GLState::generateTree() {
    treeBranches.clear();
    treeLeaves.clear();

    // Start with the trunk
    glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 direction = glm::vec3(0.0f, 1.0f, 0.0f);
    float length = treeParams.treeHeight;
    float radius = treeParams.branchThickness;

    // Create the trunk
    generateBranch(startPos, direction, length, radius, 0);

    // Update the tree geometry on the GPU
    updateTreeBuffers();
}

void GLState::generateBranch(const glm::vec3& start, const glm::vec3& direction, float length, float radius, int depth) {
    // Create this branch
    TreeBranch branch;
    branch.start = start;
    branch.end = start + direction * length;
    branch.radius = radius;
    branch.depth = depth;
    treeBranches.push_back(branch);

    // Stop if we've reached max depth
    if (depth >= treeParams.maxDepth) {
        // At the ends, add leaves
        addLeaves(branch.end, radius * 2.0f, 5 + rand() % 5);
        return;
    }

    // Create child branches with randomization
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(-15.0f, 15.0f);
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> lenDist(0.8f, 1.2f);

    // Number of child branches
    int numBranches = (depth < 2) ? 3 : 2;

    for (int i = 0; i < numBranches; i++) {
        // Only create branch if probability check passes
        if (probDist(gen) > treeParams.branchProbability && depth > 1) continue;

        // Calculate new direction with some randomness
        float branchAngle = treeParams.branchAngle + angleDist(gen);

        // Create a rotation axis perpendicular to the direction
        glm::vec3 rotAxis;
        if (depth == 0) {
            // For the first level, distribute branches evenly around trunk
            float angle = (2.0f * M_PI * i) / numBranches;
            rotAxis = glm::vec3(cos(angle), 0.0f, sin(angle));
        }
        else {
            // For higher levels, create more natural-looking distribution
            rotAxis = glm::normalize(glm::cross(direction, glm::vec3(direction.z, direction.x, direction.y)));
        }

        // Rotate the direction
        glm::vec3 newDirection = glm::rotate(direction, glm::radians(branchAngle), rotAxis);

        // Calculate new length and radius with some randomness
        float newLength = length * treeParams.branchLengthFactor * lenDist(gen);
        float newRadius = radius * treeParams.branchRadiusFactor;

        // Recursively create branch
        generateBranch(branch.end, newDirection, newLength, newRadius, depth + 1);
    }
}

void GLState::addLeaves(const glm::vec3& position, float size, int count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-0.5f, 0.5f);

    for (int i = 0; i < count; i++) {
        // Create leaf positions with some randomness around the branch endpoint
        glm::vec3 leafPos = position + glm::vec3(posDist(gen), posDist(gen), posDist(gen)) * size;
        treeLeaves.push_back(leafPos);
    }
}

void GLState::updateTreeBuffers() {
    // Create tree geometry if it doesn't exist yet
    if (treeVAO == 0) {
        glGenVertexArrays(1, &treeVAO);
        glGenBuffers(1, &treeBranchVBO);
        glGenBuffers(1, &treeLeafVBO);
        glGenBuffers(1, &treeIBO);
    }

    // First, convert branches and leaves to renderable geometry
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> indices;

    // Process each branch to create cylinder geometry
    unsigned int vertexOffset = 0;
    const int segments = 8; // Number of sides for branch cylinders

    for (const auto& branch : treeBranches) {
        glm::vec3 direction = glm::normalize(branch.end - branch.start);

        // Create a coordinate system for the branch
        glm::vec3 up = direction;
        glm::vec3 right = glm::normalize(glm::cross(up, glm::vec3(0, 0, 1)));
        if (glm::length(right) < 0.01f) {
            right = glm::normalize(glm::cross(up, glm::vec3(0, 1, 0)));
        }
        glm::vec3 forward = glm::normalize(glm::cross(right, up));

        // Generate circles at both ends of the branch
        for (int end = 0; end < 2; end++) {
            glm::vec3 center = (end == 0) ? branch.start : branch.end;
            float radius = (end == 0) ? branch.radius : branch.radius * 0.9f; // Taper the branch

            for (int i = 0; i < segments; i++) {
                float angle = 2.0f * M_PI * i / segments;
                glm::vec3 circlePos = center + (right * cos(angle) + forward * sin(angle)) * radius;
                glm::vec3 normal = glm::normalize(circlePos - center);

                positions.push_back(circlePos);
                normals.push_back(normal);
                texCoords.push_back(glm::vec2(static_cast<float>(i) / segments, static_cast<float>(end)));
            }
        }

        // Create indices for triangles
        for (int i = 0; i < segments; i++) {
            int next = (i + 1) % segments;

            // Bottom face vertices
            int bottomCurrent = vertexOffset + i;
            int bottomNext = vertexOffset + next;

            // Top face vertices
            int topCurrent = vertexOffset + segments + i;
            int topNext = vertexOffset + segments + next;

            // Triangle 1
            indices.push_back(bottomCurrent);
            indices.push_back(topCurrent);
            indices.push_back(bottomNext);

            // Triangle 2
            indices.push_back(bottomNext);
            indices.push_back(topCurrent);
            indices.push_back(topNext);
        }

        vertexOffset += segments * 2;
    }

    // Bind the geometry to OpenGL buffers
    glBindVertexArray(treeVAO);

    // Upload branch vertices
    glBindBuffer(GL_ARRAY_BUFFER, treeBranchVBO);
    glBufferData(GL_ARRAY_BUFFER,
        positions.size() * sizeof(glm::vec3) +
        normals.size() * sizeof(glm::vec3) +
        texCoords.size() * sizeof(glm::vec2),
        nullptr, GL_STATIC_DRAW);

    // Upload each attribute
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        positions.size() * sizeof(glm::vec3), positions.data());
    glBufferSubData(GL_ARRAY_BUFFER,
        positions.size() * sizeof(glm::vec3),
        normals.size() * sizeof(glm::vec3), normals.data());
    glBufferSubData(GL_ARRAY_BUFFER,
        positions.size() * sizeof(glm::vec3) + normals.size() * sizeof(glm::vec3),
        texCoords.size() * sizeof(glm::vec2), texCoords.data());

    // Set up vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
        (void*)(positions.size() * sizeof(glm::vec3)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2),
        (void*)(positions.size() * sizeof(glm::vec3) +
            normals.size() * sizeof(glm::vec3)));

    // Upload leaf positions to a separate buffer
    if (!treeLeaves.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, treeLeafVBO);
        glBufferData(GL_ARRAY_BUFFER, treeLeaves.size() * sizeof(glm::vec3),
            treeLeaves.data(), GL_STATIC_DRAW);
    }

    // Upload indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, treeIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
        indices.data(), GL_STATIC_DRAW);

    // Store the sizes for rendering
    numBranchIndices = indices.size();
    numLeaves = treeLeaves.size();

    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GLState::renderTree(const glm::mat4& viewProjMat) {
    if (treeVAO == 0 || numBranchIndices == 0) return;

    // Use the tree shader
    glUseProgram(treeShader);

    // Set up the model matrix for the tree (identity for now)
    glm::mat4 modelMat = glm::mat4(1.0f);

    // Pass uniforms to the shader
    glUniformMatrix4fv(treeModelMatLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniformMatrix4fv(treeViewProjMatLoc, 1, GL_FALSE, glm::value_ptr(viewProjMat));
    glUniform3fv(treeBarkColorLoc, 1, glm::value_ptr(treeParams.barkColor));
    glUniform3fv(treeLeafColorLoc, 1, glm::value_ptr(treeParams.leafColor));

    // Render the branches
    glBindVertexArray(treeVAO);
    glUniform1i(treeIsLeafLoc, 0); // Not a leaf
    glDrawElements(GL_TRIANGLES, numBranchIndices, GL_UNSIGNED_INT, 0);

    // Render the leaves (as points)
    if (numLeaves > 0) {
        glUniform1i(treeIsLeafLoc, 1); // Is a leaf
        glBindBuffer(GL_ARRAY_BUFFER, treeLeafVBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glPointSize(5.0f);
        glDrawArrays(GL_POINTS, 0, numLeaves);
    }

    // Reset state
    glBindVertexArray(0);
    glUseProgram(0);
}
void renderBitmapString(float x, float y, void* font, const std::string& str) {
    glRasterPos2f(x, y);
    for (char c : str) glutBitmapCharacter(font, c);
}
void GLState::renderUI() {
    if (!showUIControls) return;

    // Set up orthographic projection for UI
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, width, height, 0); // top-left is (0,0)
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    // --- Draw UI panel background ---
    glColor4f(0.97f, 0.97f, 0.97f, 0.95f);
    glBegin(GL_QUADS);
    glVertex2f(30, 30);
    glVertex2f(330, 30);
    glVertex2f(330, 500);
    glVertex2f(30, 500);
    glEnd();

    // --- Draw title ---
    glColor3f(0.07f, 0.07f, 0.07f);
    renderBitmapString(50, 70, GLUT_BITMAP_HELVETICA_18, "Features");

    // --- Draw sliders ---
    const char* labels[] = { "Branch thickness", "Some Feature", "Another one" };
    float values[] = { treeParams.branchThickness, 4.0f, 3.0f }; // Replace with your real params
    float mins[] = { 1.0f, 1.0f, 1.0f };
    float maxs[] = { 5.0f, 8.0f, 6.0f };

    for (int i = 0; i < 3; ++i) {
        int y = 120 + 80 * i;
        // Label
        glColor3f(0.07f, 0.07f, 0.07f);
        renderBitmapString(50, y, GLUT_BITMAP_HELVETICA_12, labels[i]);

        // Slider bar
        glColor3f(0.8f, 0.8f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(50, y + 20);
        glVertex2f(250, y + 20);
        glVertex2f(250, y + 32);
        glVertex2f(50, y + 32);
        glEnd();

        // Slider knob
        float norm = (values[i] - mins[i]) / (maxs[i] - mins[i]);
        float xpos = 50 + norm * 200;
        glColor3f(0.5f, 0.5f, 0.5f);
        glBegin(GL_QUADS);
        glVertex2f(xpos - 7, y + 15);
        glVertex2f(xpos + 7, y + 15);
        glVertex2f(xpos + 7, y + 37);
        glVertex2f(xpos - 7, y + 37);
        glEnd();

        // Value
        char buf[16];
        sprintf(buf, "%.1f", values[i]);
        glColor3f(0.2f, 0.2f, 0.2f);
        renderBitmapString(260, y + 30, GLUT_BITMAP_HELVETICA_12, buf);
    }

    // --- Color Picker ---
    int colorY = 380;
    renderBitmapString(50, colorY, GLUT_BITMAP_HELVETICA_12, "COLOR");
    // Color bar (rainbow)
    for (int i = 0; i < 200; ++i) {
        float hue = i / 200.0f;
        glm::vec3 color = hsvToRgb(glm::vec3(hue, 1.0f, 1.0f));
        glColor3f(color.r, color.g, color.b);
        glBegin(GL_LINES);
        glVertex2f(50 + i, colorY + 20);
        glVertex2f(50 + i, colorY + 35);
        glEnd();
    }

    // --- Brightness Picker ---
    renderBitmapString(50, colorY + 40, GLUT_BITMAP_HELVETICA_12, "BRIGHTNESS");
    for (int i = 0; i < 200; ++i) {
        float v = i / 200.0f;
        glColor3f(v, v, v);
        glBegin(GL_LINES);
        glVertex2f(50 + i, colorY + 60);
        glVertex2f(50 + i, colorY + 75);
        glEnd();
    }

    glEnable(GL_DEPTH_TEST);

    // Restore matrices
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}


void GLState::renderText(int x, int y, const char* text) {
    // Position the text
    glRasterPos2i(x, y);

    // Draw each character
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
}

glm::vec3 GLState::hsvToRgb(const glm::vec3& hsv) {
    float h = hsv.x;
    float s = hsv.y;
    float v = hsv.z;

    if (s <= 0.0f) {
        return glm::vec3(v, v, v);
    }

    float hh = h;
    if (hh >= 1.0f) hh = 0.0f;
    hh = 6.0f * hh;

    int i = (int)hh;
    float ff = hh - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - (s * ff));
    float t = v * (1.0f - (s * (1.0f - ff)));

    switch (i) {
    case 0:
        return glm::vec3(v, t, p);
    case 1:
        return glm::vec3(q, v, p);
    case 2:
        return glm::vec3(p, v, t);
    case 3:
        return glm::vec3(p, q, v);
    case 4:
        return glm::vec3(t, p, v);
    default:
        return glm::vec3(v, p, q);
    }
}

void GLState::handleUIInput(int x, int y, bool isClick) {
    if (!showUIControls) return;

    // Check if click is within UI area
    if (x < 20 || x > 300 || y < 20 || y > 300) return;

    // Check for clicks on parameter sliders
    for (int i = 0; i < 3; i++) {
        int sliderY = 80 + i * 50;

        if (y >= sliderY + 15 && y <= sliderY + 35) {
            // Click on this slider
            selectedParameter = i;

            if (x >= 30 && x <= 250 && isClick) {
                // Calculate new parameter value based on click position
                float normalizedValue = (x - 30) / 220.0f;

                switch (i) {
                case 0: // Branch thickness
                    treeParams.branchThickness = 0.1f + normalizedValue * 4.9f;
                    break;
                case 1: // Tree height
                    treeParams.treeHeight = 1.0f + normalizedValue * 9.0f;
                    break;
                case 2: // Branch angle
                    treeParams.branchAngle = 10.0f + normalizedValue * 50.0f;
                    break;
                }

                // Regenerate tree with new parameters
                generateTree();
            }
        }
    }

    // Check for clicks on color picker
    int colorY = 230;
    if (y >= colorY + 20 && y <= colorY + 40 && x >= 30 && x <= 230 && isClick) {
        // Calculate selected hue
        float hue = (x - 30) / 200.0f;
        treeParams.leafColor = hsvToRgb(glm::vec3(hue, 1.0f, 1.0f));
        generateTree();
    }
}

void GLState::adjustSelectedParameter(bool increase) {
    float delta = increase ? 0.1f : -0.1f;

    switch (selectedParameter) {
    case 0: // Branch thickness
        treeParams.branchThickness = glm::clamp(treeParams.branchThickness + delta, 0.1f, 5.0f);
        break;
    case 1: // Tree height
        treeParams.treeHeight = glm::clamp(treeParams.treeHeight + delta, 1.0f, 10.0f);
        break;
    case 2: // Branch angle
        treeParams.branchAngle = glm::clamp(treeParams.branchAngle + delta * 10.0f, 10.0f, 60.0f);
        break;
    }

    // Regenerate tree with new parameters
    generateTree();
}

// Called when OpenGL context is created (some time after construction)
void GLState::initializeGL() {
    // General settings
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Initialize OpenGL state
    initShaders();

    // Set drawing state
    setShadingMode(SHADINGMODE_PHONG);
    setNormalMapMode(NORMAL_MAPPING_ON);
    setShadowMapMode(SHADOW_MAPPING_ON);

    // Create lights
    lights.resize(Light::MAX_LIGHTS);

    // Initialize textures
    textures.load();
    textures.prepareDepthMap();

    // Generate initial tree
    generateTree();

    // Set initialized state
    init = true;
}

// Called when window requests a screen redraw
void GLState::paintGL() {
    // Clear the color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ========== Begin the first render pass to generate the depth map ==========
    glUseProgram(depthShader);

    // Render the scene from the light's perspective
    glm::mat4 lightSpaceMat = glm::mat4(1.0f);
    glm::mat4 lightProj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 20.0f);
    glm::mat4 lightView = glm::lookAt(lights[0].getPos(), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    lightSpaceMat = lightProj * lightView;

    // Pass the transform matrix to the depth shader
    glUniformMatrix4fv(lightSpaceMatDepthLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMat));

    // Prepare before rendering
    int shadowWidth, shadowHeight;
    textures.getShadowWidthHeight(shadowWidth, shadowHeight);
    glViewport(0, 0, shadowWidth, shadowHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, textures.getdepthMapFBO());
    glClear(GL_DEPTH_BUFFER_BIT);

    glCullFace(GL_FRONT);  // Fix peter panning
    for (auto& objPtr : objects) {
        // Pass the model matrix to the depth shader
        glm::mat4 modelMat = objPtr->getModelMat();
        glUniformMatrix4fv(modelMatDepthLoc, 1, GL_FALSE, glm::value_ptr(modelMat));

        // Draw the mesh
        objPtr->draw();
    }
    glCullFace(GL_BACK);  // Reset
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);

    // ========== Begin the second render pass ===================================
    glViewport(0, 0, width, height);  // Reset the viewport
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader);

    // Activate textures and pass them to the shader
    textures.activeTextures();
    textures.activeDepthMap();
    GLuint texUnitLoc0, texUnitLoc1, texUnitLoc2, texUnitLoc3;
    texUnitLoc0 = glGetUniformLocation(shader, "texPlane");
    texUnitLoc1 = glGetUniformLocation(shader, "texCube");
    texUnitLoc2 = glGetUniformLocation(shader, "texCubeNorm");
    texUnitLoc3 = glGetUniformLocation(shader, "shadowMap");
    glUniform1i(texUnitLoc0, 0);
    glUniform1i(texUnitLoc1, 1);
    glUniform1i(texUnitLoc2, 2);
    glUniform1i(texUnitLoc3, 3);

    // Pass the transform matrix to the shader
    glUniformMatrix4fv(lightSpaceMatLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMat));

    // Construct a transformation matrix for the camera
    glm::mat4 viewProjMat(1.0f);
    // Perspective projection
    float aspect = (float)width / (float)height;
    glm::mat4 proj = glm::perspective(glm::radians(fovy), aspect, 0.1f, 100.0f);
    // Camera viewpoint
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, -camCoords.z));
    view = glm::rotate(view, glm::radians(camCoords.y), glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::rotate(view, glm::radians(camCoords.x), glm::vec3(0.0f, 1.0f, 0.0f));
    // Combine transformations
    viewProjMat = proj * view;

    for (auto& objPtr : objects) {
        glm::mat4 modelMat = objPtr->getModelMat();
        // Upload transform matrices to shader
        glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
        glUniformMatrix4fv(viewProjMatLoc, 1, GL_FALSE, glm::value_ptr(viewProjMat));

        // Get camera position and upload to shader
        glm::vec3 camPos = glm::vec3(glm::inverse(view)[3]);
        glUniform3fv(camPosLoc, 1, glm::value_ptr(camPos));

        // Pass object type to shader
        glUniform1i(objTypeLoc, (int)objPtr->getMeshType());

        // Draw the mesh
        objPtr->draw();
    }

    glUseProgram(0);

    // Draw enabled light icons (if in lighting mode)
    if (shadingMode != SHADINGMODE_NORMALS)
        for (auto& l : lights)
            if (l.getEnabled()) l.drawIcon(viewProjMat);

    // Render the procedural tree
    renderTree(viewProjMat);

    // Render UI elements
    renderUI();
}

// Called when window is resized
void GLState::resizeGL(int w, int h) {
    // Tell OpenGL the new dimensions of the window
    width = w;
    height = h;
    glViewport(0, 0, w, h);
}

// Set the shading mode (normals or lighting)
void GLState::setShadingMode(ShadingMode sm) {
    shadingMode = sm;

    // Update mode in shader
    glUseProgram(shader);
    glUniform1i(shadingModeLoc, (int)shadingMode);
    glUseProgram(0);
}

void GLState::setNormalMapMode(NormalMapMode nmm) {
    normalMapMode = nmm;

    // Update mode in shader
    glUseProgram(shader);
    glUniform1i(normalMapModeLoc, (int)normalMapMode);
    glUseProgram(0);
}

void GLState::setShadowMapMode(ShadowMapMode smm) {
    shadowMapMode = smm;

    // Update mode in shader
    glUseProgram(shader);
    glUniform1i(shadowMapModeLoc, (int)shadowMapMode);
    glUseProgram(0);
}

// Get object color
glm::vec3 GLState::getObjectColor() const {
    glm::vec3 objColor;
    glGetUniformfv(shader, cubeColorLoc, glm::value_ptr(objColor));
    return objColor;
}

// Get ambient strength
float GLState::getAmbientStrength() const {
    float ambStr;
    glGetUniformfv(shader, cubeAmbStrLoc, &ambStr);
    return ambStr;
}

// Get diffuse strength
float GLState::getDiffuseStrength() const {
    float diffStr;
    glGetUniformfv(shader, cubeDiffStrLoc, &diffStr);
    return diffStr;
}

// Get specular strength
float GLState::getSpecularStrength() const {
    float specStr;
    glGetUniformfv(shader, cubeSpecStrLoc, &specStr);
    return specStr;
}

// Get specular exponent
float GLState::getSpecularExponent() const {
    float specExp;
    glGetUniformfv(shader, cubeSpecExpLoc, &specExp);
    return specExp;
}

// Set object color
void GLState::setObjectColor(glm::vec3 color) {
    // Update value in shader
    glUseProgram(shader);
    glUniform3fv(cubeColorLoc, 1, glm::value_ptr(color));
    glUseProgram(0);
}

// Set ambient strength
void GLState::setAmbientStrength(float ambStr) {
    // Update value in shader
    glUseProgram(shader);
    glUniform1f(cubeAmbStrLoc, ambStr);
    glUseProgram(0);
}

// Set diffuse strength
void GLState::setDiffuseStrength(float diffStr) {
    // Update value in shader
    glUseProgram(shader);
    glUniform1f(cubeDiffStrLoc, diffStr);
    glUseProgram(0);
}

// Set specular strength
void GLState::setSpecularStrength(float specStr) {
    // Update value in shader
    glUseProgram(shader);
    glUniform1f(cubeSpecStrLoc, specStr);
    glUseProgram(0);
}

// Set specular exponent
void GLState::setSpecularExponent(float specExp) {
    // Update value in shader
    glUseProgram(shader);
    glUniform1f(cubeSpecExpLoc, specExp);
    glUseProgram(0);
}

void GLState::setMaterialAttrs(
    glm::vec3 floorColor, glm::vec3 cubeColor,
    float floorAmbStr, float floorDiffStr, float floorSpecStr, float floorSpecExp,
    float cubeAmbStr, float cubeDiffStr, float cubeSpecStr, float cubeSpecExp) {  // set material attributes (initialization)
    // Update values in shader
    glUseProgram(shader);
    glUniform3fv(floorColorLoc, 1, glm::value_ptr(floorColor));
    glUniform3fv(cubeColorLoc, 1, glm::value_ptr(cubeColor));
    glUniform1f(floorAmbStrLoc, floorAmbStr);
    glUniform1f(floorDiffStrLoc, floorDiffStr);
    glUniform1f(floorSpecStrLoc, floorSpecStr);
    glUniform1f(floorSpecExpLoc, floorSpecExp);
    glUniform1f(cubeAmbStrLoc, cubeAmbStr);
    glUniform1f(cubeDiffStrLoc, cubeDiffStr);
    glUniform1f(cubeSpecStrLoc, cubeSpecStr);
    glUniform1f(cubeSpecExpLoc, cubeSpecExp);
    glUseProgram(0);
}

// Start rotating the camera (click + drag)
void GLState::beginCameraRotate(glm::vec2 mousePos) {
    camRotating = true;
    initCamRot = glm::vec2(camCoords);
    initMousePos = mousePos;
}

// Stop rotating the camera (mouse button is released)
void GLState::endCameraRotate() {
    camRotating = false;
}

// Use mouse delta to determine new camera rotation
void GLState::rotateCamera(glm::vec2 mousePos) {
    if (camRotating) {
        float rotScale = glm::min(width / 450.0f, height / 270.0f);
        glm::vec2 mouseDelta = mousePos - initMousePos;
        glm::vec2 newAngle = initCamRot + mouseDelta / rotScale;
        newAngle.y = glm::clamp(newAngle.y, -90.0f, 90.0f);
        while (newAngle.x > 180.0f) newAngle.x -= 360.0f;
        while (newAngle.x < -180.0f) newAngle.x += 360.0f;
        if (glm::length(newAngle - glm::vec2(camCoords)) > FLT_EPSILON) {
            camCoords.x = newAngle.x;
            camCoords.y = newAngle.y;
        }
    }
}

void GLState::update_time(float time) {
    cur_time = time;
}

// Moves the camera toward / away from the origin (scroll wheel)
void GLState::offsetCamera(float offset) {
    camCoords.z = glm::clamp(camCoords.z + offset, 0.1f, 10.0f);
}

// Display a given .obj file
void GLState::showObjFile(const std::string& filename, const unsigned int meshType, const glm::mat4& modelMat) {
    // Load the .obj file if it's not already loaded
    auto mesh = std::make_shared<Mesh>(filename, static_cast<Mesh::ObjType>(meshType));
    mesh->setModelMat(modelMat);
    objects.push_back(mesh);
}

// Create shaders and associated state
void GLState::initShaders() {
    // Compile and link shader files for the main rendering
    std::vector<GLuint> shaders;
    shaders.push_back(compileShader(GL_VERTEX_SHADER, "shaders/v.glsl"));
    shaders.push_back(compileShader(GL_FRAGMENT_SHADER, "shaders/f.glsl"));
    shader = linkProgram(shaders);

    // Compile and link depth shader
    std::vector<GLuint> depthShaders;
    depthShaders.push_back(compileShader(GL_VERTEX_SHADER, "shaders/depth_v.glsl"));
    depthShaders.push_back(compileShader(GL_FRAGMENT_SHADER, "shaders/depth_f.glsl"));
    depthShader = linkProgram(depthShaders);

    // Compile and link tree shader
    std::vector<GLuint> treeShaders;
    treeShaders.push_back(compileShader(GL_VERTEX_SHADER, "shaders/tree_v.glsl"));
    treeShaders.push_back(compileShader(GL_FRAGMENT_SHADER, "shaders/tree_f.glsl"));
    treeShader = linkProgram(treeShaders);

    // Cleanup extra state
    for (auto s : shaders)
        glDeleteShader(s);
    shaders.clear();

    for (auto s : depthShaders)
        glDeleteShader(s);
    depthShaders.clear();

    for (auto s : treeShaders)
        glDeleteShader(s);
    treeShaders.clear();

    // Get uniform locations for shader
    modelMatLoc = glGetUniformLocation(shader, "modelMat");
    lightSpaceMatLoc = glGetUniformLocation(shader, "lightSpaceMat");
    objTypeLoc = glGetUniformLocation(shader, "objType");
    viewProjMatLoc = glGetUniformLocation(shader, "viewProjMat");
    shadingModeLoc = glGetUniformLocation(shader, "shadingMode");
    normalMapModeLoc = glGetUniformLocation(shader, "normalMapMode");
    shadowMapModeLoc = glGetUniformLocation(shader, "shadowMapMode");
    camPosLoc = glGetUniformLocation(shader, "camPos");
    floorColorLoc = glGetUniformLocation(shader, "floorColor");
    floorAmbStrLoc = glGetUniformLocation(shader, "floorAmbStr");
    floorDiffStrLoc = glGetUniformLocation(shader, "floorDiffStr");
    floorSpecStrLoc = glGetUniformLocation(shader, "floorSpecStr");
    floorSpecExpLoc = glGetUniformLocation(shader, "floorSpecExp");
    cubeColorLoc = glGetUniformLocation(shader, "cubeColor");
    cubeAmbStrLoc = glGetUniformLocation(shader, "cubeAmbStr");
    cubeDiffStrLoc = glGetUniformLocation(shader, "cubeDiffStr");
    cubeSpecStrLoc = glGetUniformLocation(shader, "cubeSpecStr");
    cubeSpecExpLoc = glGetUniformLocation(shader, "cubeSpecExp");

    // Get uniform locations for depth shader
    modelMatDepthLoc = glGetUniformLocation(depthShader, "modelMat");
    lightSpaceMatDepthLoc = glGetUniformLocation(depthShader, "lightSpaceMat");

    // Get uniform locations for tree shader
    treeModelMatLoc = glGetUniformLocation(treeShader, "modelMat");
    treeViewProjMatLoc = glGetUniformLocation(treeShader, "viewProjMat");
    treeBarkColorLoc = glGetUniformLocation(treeShader, "barkColor");
    treeLeafColorLoc = glGetUniformLocation(treeShader, "leafColor");
    treeIsLeafLoc = glGetUniformLocation(treeShader, "isLeaf");

    // Bind lights uniform block to binding index
    glUseProgram(shader);
    GLuint lightBlockIndex = glGetUniformBlockIndex(shader, "LightBlock");
    glUniformBlockBinding(shader, lightBlockIndex, Light::BIND_PT);
    glUseProgram(0);

    // Create shader files for the tree if they don't exist
    createTreeShaderFiles();
}

void GLState::createTreeShaderFiles() {
    // Create vertex shader for tree rendering
    std::string treeVertexShader =
        "#version 330 core\n"
        "layout(location = 0) in vec3 position;\n"
        "layout(location = 1) in vec3 normal;\n"
        "layout(location = 2) in vec2 texCoords;\n"
        "\n"
        "uniform mat4 modelMat;\n"
        "uniform mat4 viewProjMat;\n"
        "uniform bool isLeaf;\n"
        "\n"
        "out vec3 fragNormal;\n"
        "out vec3 fragPos;\n"
        "out vec2 fragTexCoords;\n"
        "out bool isLeafFrag;\n"
        "\n"
        "void main() {\n"
        "    fragPos = vec3(modelMat * vec4(position, 1.0));\n"
        "    fragNormal = mat3(transpose(inverse(modelMat))) * normal;\n"
        "    fragTexCoords = texCoords;\n"
        "    isLeafFrag = isLeaf;\n"
        "    gl_Position = viewProjMat * modelMat * vec4(position, 1.0);\n"
        "}\n";

    // Create fragment shader for tree rendering
    std::string treeFragmentShader =
        "#version 330 core\n"
        "in vec3 fragNormal;\n"
        "in vec3 fragPos;\n"
        "in vec2 fragTexCoords;\n"
        "in bool isLeafFrag;\n"
        "\n"
        "uniform vec3 barkColor;\n"
        "uniform vec3 leafColor;\n"
        "\n"
        "out vec4 fragColor;\n"
        "\n"
        "void main() {\n"
        "    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));\n"
        "    vec3 normal = normalize(fragNormal);\n"
        "    \n"
        "    // Simple lighting calculation\n"
        "    float diff = max(dot(normal, lightDir), 0.0);\n"
        "    vec3 diffuse = diff * vec3(1.0);\n"
        "    vec3 ambient = vec3(0.3);\n"
        "    \n"
        "    // Use different color for bark and leaves\n"
        "    vec3 baseColor = isLeafFrag ? leafColor : barkColor;\n"
        "    \n"
        "    fragColor = vec4(baseColor * (ambient + diffuse), 1.0);\n"
        "}\n";

    // Write files to disk
    std::ofstream vertFile("shaders/tree_v.glsl");
    if (vertFile.is_open()) {
        vertFile << treeVertexShader;
        vertFile.close();
    }

    std::ofstream fragFile("shaders/tree_f.glsl");
    if (fragFile.is_open()) {
        fragFile << treeFragmentShader;
        fragFile.close();
    }
}

// Trim leading and trailing whitespace from a line
std::string trim(const std::string& line) {
    const std::string whitespace = " \t\r\n";
    auto first = line.find_first_not_of(whitespace);
    if (first == std::string::npos)
        return "";
    auto last = line.find_last_not_of(whitespace);
    auto range = last - first + 1;
    return line.substr(first, range);
}

// Reads lines from istream, stripping whitespace and comments,
// until it finds a line with content in it
std::string getNextLine(std::istream& istr) {
    const std::string comment = "#";
    std::string line = "";
    while (line == "") {
        std::getline(istr, line);
        // Skip comments and empty lines
        auto found = line.find(comment);
        if (found != std::string::npos)
            line = line.substr(0, found);
        line = trim(line);
    }
    return line;
}

// Preprocess the file to remove empty lines and comments
std::string preprocessFile(std::string filename) {
    std::ifstream file;
    file.exceptions(std::ios::badbit | std::ios::failbit);
    file.open(filename);

    std::stringstream ss;
    try {
        // Read each line until the end of the file
        while (true) {
            std::string line = getNextLine(file);
            ss << line << std::endl;
        }
    }
    catch (const std::exception& e) { e; }

    return ss.str();
}

// Read config file
void GLState::readConfig(std::string filename) {
    try {
        // Read the file contents into a string stream
        std::stringstream ss;
        ss.str(preprocessFile(filename));
        ss.exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);

        ss >> numObjects;  // get number of objects
        std::vector<std::pair<std::string, unsigned int>> objNamesTypes;  // model names and their types
        for (unsigned int i = 0; i < numObjects; i++) {
            // Read .obj filename
            std::string objName;
            unsigned int meshType;
            ss >> objName;
            ss >> meshType;
            objNamesTypes.push_back(std::make_pair(objName, meshType));
        }

        for (auto& objNameType : objNamesTypes) {
            std::string objName = objNameType.first;
            unsigned int meshType = objNameType.second;

            glm::mat3 rotMat;  // rotation matrix
            glm::vec3 translation;  // translation vector
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ss >> rotMat[i][j];
                }
            }
            for (int i = 0; i < 3; i++)
                ss >> translation[i];

            glm::mat4 modelMat = calModelMat(rotMat, translation);  // model matrix
            showObjFile(objName, static_cast<Mesh::ObjType>(meshType), modelMat);  // add this object to the scene
        }

        // Objects attributes
        float cubeAmbStr, cubeDiffStr, cubeSpecStr, cubeSpecExp;
        float floorAmbStr, floorDiffStr, floorSpecStr, floorSpecExp;
        glm::vec3 cubeColor, floorColor;
        // Read material properties
        ss >> cubeAmbStr >> cubeDiffStr >> cubeSpecStr >> cubeSpecExp;
        ss >> cubeColor.r >> cubeColor.g >> cubeColor.b;
        cubeColor /= 255.0f;
        ss >> floorAmbStr >> floorDiffStr >> floorSpecStr >> floorSpecExp;
        ss >> floorColor.r >> floorColor.g >> floorColor.b;
        floorColor /= 255.0f;

        // Set material properties
        setMaterialAttrs(
            floorColor, cubeColor,
            floorAmbStr, floorDiffStr, floorSpecStr, floorSpecExp,
            cubeAmbStr, cubeDiffStr, cubeSpecStr, cubeSpecExp
        );

        // Read number of lights
        unsigned int numLights;
        ss >> numLights;
        if (numLights == 0)
            throw std::runtime_error("Must have at least 1 light");
        if (numLights > Light::MAX_LIGHTS)
            throw std::runtime_error("Cannot create more than "
                + std::to_string(Light::MAX_LIGHTS) + " lights");

        for (unsigned int i = 0; i < lights.size(); i++) {
            // Read properties of each light
            if (i < numLights) {
                int enabled, type;
                glm::vec3 lightColor, lightPos;
                ss >> enabled >> type;
                ss >> lightColor.r >> lightColor.g >> lightColor.b;
                ss >> lightPos.x >> lightPos.y >> lightPos.z;
                lightColor /= 255.0f;
                lights[i].setEnabled((bool)enabled);
                lights[i].setType((Light::LightType)type);
                lights[i].setColor(lightColor);
                lights[i].setPos(lightPos);

                // Disable all other lights
            }
            else
                lights[i].setEnabled(false);
        }

    }
    catch (const std::exception& e) {
        // Construct an error message and throw again
        std::stringstream ss;
        ss << "Failed to read config file " << filename << ": " << e.what();
        throw std::runtime_error(ss.str());
    }
}

glm::mat4 GLState::calModelMat(const glm::mat3 rotMat, const glm::vec3 translation) {
    glm::mat4 modelMat = glm::mat4(1.0);  // initialize the matrix as an identity matrix
    glm::mat4 rotateMat = glm::mat4(1.0);
    glm::mat4 translateMat = glm::mat4(1.0);
    // copy
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            rotateMat[i][j] = rotMat[i][j];
        }
    }
    for (int i = 0; i < 3; i++)
        translateMat[i][3] = translation[i];
    modelMat = glm::transpose(translateMat) * glm::transpose(rotateMat);
    return modelMat;
}
