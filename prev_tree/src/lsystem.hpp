#ifndef LSYSTEM_HPP
#define LSYSTEM_HPP

#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <map>
#include <glm/glm.hpp>
#include "gl_core_3_3.h"

class LSystem {
public:
    LSystem();
    ~LSystem();
    // Move constructor and assignment
    LSystem(LSystem&& other);
    LSystem& operator=(LSystem&& other);
    // Disallow copy
    LSystem(const LSystem& other) = delete;
    LSystem& operator=(const LSystem& other) = delete;

    void parse(std::istream& istr);
    void parseString(std::string string);
    void parseFile(std::string filename);

    unsigned int iterate();

    void draw(glm::mat4 viewProj);
    void drawIter(unsigned int iter, glm::mat4 viewProj);

    unsigned int getNumIter() const { return strings.size(); }
    std::string getString(unsigned int iter) const { return strings.at(iter); }

private:
    std::string applyRules(std::string string);
    // lsystem.hpp
    std::vector<glm::vec3> createGeometry(const std::string& string);
    void addVerts(std::vector<glm::vec3>& verts);


    std::vector<std::string> strings;
    std::multimap<char, std::pair<std::string, float>> rules;
    float angle, jitter;
    std::mt19937 rng;

    struct IterData {
        GLint first;
        GLsizei count;
        glm::mat4 bbfix;
    };

    static const GLsizei MAX_BUF = 1 << 26;
    GLuint vao, vbo;
    std::vector<IterData> iterData;
    GLsizei bufSize;


    static unsigned int refcount;
    static GLuint shader;
    static GLuint xformLoc;
    void initShader();
};

#endif