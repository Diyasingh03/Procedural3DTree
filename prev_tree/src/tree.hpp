// tree.hpp
#pragma once
#include <vector>
#include <glm/glm.hpp>

struct Branch {
    glm::vec3 start;
    glm::vec3 end;
    float radius;
};

class Tree {
public:
    // Parameters that can be adjusted via UI
    struct Parameters {
        float branchThickness = 2.0f;
        float height = 5.0f;
        float branchAngle = 30.0f;
        glm::vec3 leafColor = glm::vec3(1.0f, 0.5f, 0.5f); // Pink cherry blossom
        glm::vec3 barkColor = glm::vec3(0.6f, 0.3f, 0.2f); // Brown bark
    };

    Tree();
    ~Tree();

    void generateTree(const Parameters& params);
    void render();

private:
    std::vector<Branch> branches;
    GLuint vao, vbo;

    void createBranch(const glm::vec3& start, const glm::vec3& direction,
        float length, float radius, int depth);
};
