#define NOMINMAX
#include "lsystem.hpp"
#include <fstream>
#include <sstream>
#include <stack>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include "util.hpp"

// Stream helpers
std::stringstream preprocessStream(std::istream& istr);
std::string getNextLine(std::istream& istr);
std::string trim(const std::string& line);

unsigned int LSystem::refcount = 0;
GLuint LSystem::shader = 0;
GLuint LSystem::xformLoc = 0;

LSystem::LSystem() :
    angle(0.0f), vao(0), vbo(0), bufSize(0)
{
    std::random_device rd;
    rng = std::mt19937(rd());
    if (refcount == 0)
        initShader();
    refcount++;
}

LSystem::~LSystem() {
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    bufSize = 0;
    refcount--;
    if (refcount == 0) {
        if (shader) { glDeleteProgram(shader); shader = 0; }
    }
}

LSystem::LSystem(LSystem&& other) :
    strings(std::move(other.strings)),
    rules(std::move(other.rules)),
    angle(other.angle),
    //jitter(other.jitter),
    vao(other.vao),
    vbo(other.vbo),
    iterData(std::move(other.iterData)),
    bufSize(other.bufSize)
{
    other.vao = 0;
    other.vbo = 0;
    other.bufSize = 0;
    refcount++;
}

LSystem& LSystem::operator=(LSystem&& other) {
    strings = std::move(other.strings);
    rules = std::move(other.rules);
    angle = other.angle;
    //jitter = other.jitter;
    iterData = std::move(other.iterData);
    bufSize = other.bufSize;
    if (vao) { glDeleteVertexArrays(1, &vao); }
    if (vbo) { glDeleteBuffers(1, &vbo); }
    vao = other.vao;
    vbo = other.vbo;
    other.vao = 0;
    other.vbo = 0;
    other.bufSize = 0;
    return *this;
}

void LSystem::parse(std::istream& istr) {
    float inAngle = 0.0f;
    float inJitter = 0.0f;
    unsigned int inIters = 0;
    std::string inAxiom;
    std::multimap<char, std::pair<std::string, float>> inRules;

    std::string line = getNextLine(istr);
    std::string angleLine = line;
    line = getNextLine(istr);
    inIters = std::stoi(line);
    line = getNextLine(istr);
    inAxiom = line;
    while (true) {
        try {
            std::getline(istr, line);
            if (line.empty()) break;
            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos) {
                throw std::runtime_error("missing ':'");
            }
            std::string key = trim(line.substr(0, colonPos));
            std::string value = trim(line.substr(colonPos + 1));
            float weight = 1.0f;
            size_t weightPos = value.find(' ');
            if (weightPos != std::string::npos) {
                weight = std::stof(value.substr(weightPos + 1));
                value = value.substr(0, weightPos);
            }
            inRules.emplace(key[0], std::make_pair(value, weight));
        }
        catch (const std::exception& e) {
            if (istr.eof()) break;
            throw;

        }
    }

    size_t pos = angleLine.find("<");
    if (pos != std::string::npos) {
        size_t endPos = angleLine.find(">");
        std::string angleStr = angleLine.substr(0, pos);
        std::string jitterStr = angleLine.substr(pos + 1, endPos - pos - 1);
        inAngle = std::stof(angleStr);
        inJitter = std::stof(jitterStr);
    }
    else {
        inAngle = std::stof(angleLine);
    }
    //debug
    std::cout << "angle: " << inAngle << std::endl;
    std::cout << "jitter: " << inJitter << std::endl;
    std::cout << "iterations: " << inIters << std::endl;
    std::cout << "axiom: " << inAxiom << std::endl;
    std::cout << "rules: " << std::endl;
    for (auto& rule : inRules) {
        std::cout << "  " << rule.first << ": " << rule.second.first;
        if (rule.second.second != 1.0f) {
            std::cout << " (" << rule.second.second << ")";
        }
        std::cout << std::endl;
    }


    angle = inAngle;
    jitter = inJitter;
    strings = { inAxiom };
    rules = std::move(inRules);
    iterData.clear();
    auto verts = createGeometry(strings.back());
    addVerts(verts);

    try {
        while (strings.size() < inIters)
            iterate();
    }
    catch (const std::exception& e) {
        std::cerr << "Too many iterations: geometry exceeds maximum buffer size" << std::endl;
    }
}

void LSystem::parseString(std::string string) {
    std::stringstream ss(string);
    ss = preprocessStream(ss);
    parse(ss);
}

void LSystem::parseFile(std::string filename) {
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("failed to open " + filename);
    std::stringstream ss = preprocessStream(file);
    parse(ss);
}

unsigned int LSystem::iterate() {
    if (strings.empty()) return 0;
    std::string newString = applyRules(strings.back());
    auto verts = createGeometry(newString);
    auto& id = iterData.back();
    if ((id.first + id.count + verts.size()) * sizeof(glm::vec3) > MAX_BUF)
        throw std::runtime_error("geometry exceeds maximum buffer size");
    strings.push_back(newString);
    addVerts(verts);
    return getNumIter();
}

void LSystem::draw(glm::mat4 viewProj) {
    if (!getNumIter()) return;
    drawIter(getNumIter() - 1, viewProj);
}

void LSystem::drawIter(unsigned int iter, glm::mat4 viewProj) {
    IterData& id = iterData.at(iter);
    glUseProgram(shader);
    glBindVertexArray(vao);
    glm::mat4 xform = viewProj * id.bbfix;
    glUniformMatrix4fv(xformLoc, 1, GL_FALSE, glm::value_ptr(xform));
    glDrawArrays(GL_LINES, id.first, id.count);
    glBindVertexArray(0);
    glUseProgram(0);
}

std::string LSystem::applyRules(std::string string) {
    std::string newString = "";
    for (char c : string) {
        if (rules.find(c) == rules.end()) {
            newString += c;
        }
        else {
            auto range = rules.equal_range(c);
            float totalWeight = 0.0f;
            for (auto it = range.first; it != range.second; ++it) {
                totalWeight += it->second.second;
            }
            std::uniform_real_distribution<float> dist(0.0f, totalWeight);
            float randomWeight = dist(rng);
            float cumulativeWeight = 0.0f;
            for (auto it = range.first; it != range.second; ++it) {
                cumulativeWeight += it->second.second;
                if (randomWeight <= cumulativeWeight) {
                    newString += it->second.first;
                    break;
                }
            }
        }
    }
    return newString;
}

// --- 3D Turtle Geometry ---
std::vector<glm::vec3> LSystem::createGeometry(const std::string& string) {
    std::vector<glm::vec3> verts;
    glm::vec3 pos(0.0f), dir(0.0f, 1.0f, 0.0f), up(0.0f, 0.0f, 1.0f), right(1.0f, 0.0f, 0.0f);
    std::stack<std::tuple<glm::vec3, glm::vec3, glm::vec3, glm::vec3>> stack;

    for (size_t i = 0; i < string.size(); ++i) {
        char c = string[i];
        if (tolower(c) == 'f' || tolower(c) == 'g') {
            glm::vec3 newPos = pos + dir;
            verts.push_back(pos);
            verts.push_back(newPos);
            pos = newPos;
        }
        else if (tolower(c) == 's') {
            pos += dir;
        }
        else if (c == '+') { // Yaw left (around up)
            float currangle = glm::radians(angle);
            if (jitter != 0.0f) {
                std::uniform_real_distribution<float> dist(-jitter, jitter);
                currangle += glm::radians(dist(rng));
            }
            dir = glm::rotate(dir, currangle, up);
            right = glm::rotate(right, currangle, up);
        }
        else if (c == '-') { // Yaw right (around up)
            float currangle = glm::radians(-angle);
            if (jitter != 0.0f) {
                std::uniform_real_distribution<float> dist(-jitter, jitter);
                currangle += glm::radians(dist(rng));
            }
            dir = glm::rotate(dir, currangle, up);
            right = glm::rotate(right, currangle, up);
        }
        else if (c == '&') { // Pitch down (around right)
            float currangle = glm::radians(angle);
            if (jitter != 0.0f) {
                std::uniform_real_distribution<float> dist(-jitter, jitter);
                currangle += glm::radians(dist(rng));
            }
            dir = glm::rotate(dir, currangle, right);
            up = glm::rotate(up, currangle, right);
        }
        else if (c == '^') { // Pitch up (around right)
            float currangle = glm::radians(-angle);
            if (jitter != 0.0f) {
                std::uniform_real_distribution<float> dist(-jitter, jitter);
                currangle += glm::radians(dist(rng));
            }
            dir = glm::rotate(dir, currangle, right);
            up = glm::rotate(up, currangle, right);
        }
        else if (c == '/') { // Roll left (around dir)
            float currangle = glm::radians(angle);
            if (jitter != 0.0f) {
                std::uniform_real_distribution<float> dist(-jitter, jitter);
                currangle += glm::radians(dist(rng));
            }
            right = glm::rotate(right, currangle, dir);
            up = glm::rotate(up, currangle, dir);
        }
        else if (c == '\\') { // Roll right (around dir)
            float currangle = glm::radians(-angle);
            if (jitter != 0.0f) {
                std::uniform_real_distribution<float> dist(-jitter, jitter);
                currangle += glm::radians(dist(rng));
            }
            right = glm::rotate(right, currangle, dir);
            up = glm::rotate(up, currangle, dir);
        }
        else if (c == '|') { // Turn 180 degrees
            dir = -dir;
            right = -right;
        }
        else if (c == '[') { // Push state
            stack.push(std::make_tuple(pos, dir, up, right));
        }
        else if (c == ']') { // Pop state
            if (!stack.empty()) {
                std::tie(pos, dir, up, right) = stack.top();
                stack.pop();
            }
        }
    }
    return verts;
}


void LSystem::addVerts(std::vector<glm::vec3>& verts) {
    IterData id;
    if (iterData.empty())
        id.first = 0;
    else {
        auto& lastID = iterData.back();
        id.first = lastID.first + lastID.count;
    }
    id.count = verts.size();

    // 3D bounding box
    glm::vec3 minBB(std::numeric_limits<float>::max());
    glm::vec3 maxBB(std::numeric_limits<float>::lowest());
    for (auto& v : verts) {
        minBB = glm::min(minBB, v);
        maxBB = glm::max(maxBB, v);
    }
    glm::vec3 diag = maxBB - minBB;
    float scale = 1.9f / glm::max(glm::max(diag.x, diag.y), diag.z);
    id.bbfix = glm::mat4(1.0f);
    id.bbfix[0][0] = scale;
    id.bbfix[1][1] = scale;
    id.bbfix[2][2] = scale;
    id.bbfix[3] = glm::vec4(-(minBB + maxBB) * scale / 2.0f, 1.0f);
    iterData.push_back(id);

    GLsizei newSize = (id.first + id.count) * sizeof(glm::vec3);
    if (newSize > bufSize) {
        GLuint tempBuf;
        glGenBuffers(1, &tempBuf);
        glBindBuffer(GL_ARRAY_BUFFER, tempBuf);
        glBufferData(GL_ARRAY_BUFFER, newSize, nullptr, GL_STATIC_DRAW);
        if (vbo) {
            glBindBuffer(GL_COPY_READ_BUFFER, vbo);
            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER, 0, 0, bufSize);
            glBindBuffer(GL_COPY_READ_BUFFER, 0);
            glDeleteBuffers(1, &vbo);
        }
        vbo = tempBuf;
        bufSize = newSize;
    }
    else
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferSubData(GL_ARRAY_BUFFER, id.first * sizeof(glm::vec3), id.count * sizeof(glm::vec3), verts.data());

    if (!vao) {
        glGenVertexArrays(1, &vao);
    }
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (GLvoid*)0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void LSystem::initShader() {
    std::vector<GLuint> shaders;
    shaders.push_back(compileShader(GL_VERTEX_SHADER, "shaders/v.glsl"));
    shaders.push_back(compileShader(GL_FRAGMENT_SHADER, "shaders/f.glsl"));
    shader = linkProgram(shaders);
    for (auto s : shaders)
        glDeleteShader(s);
    shaders.clear();
    xformLoc = glGetUniformLocation(shader, "xform");
}

std::stringstream preprocessStream(std::istream& istr) {
    istr.exceptions(istr.badbit | istr.failbit);
    std::stringstream ss;
    try {
        while (true) {
            std::string line = getNextLine(istr);
            ss << line << std::endl;
        }
    }
    catch (const std::exception& e) {
        if (!istr.eof()) throw e;
    }
    return ss;
}

std::string getNextLine(std::istream& istr) {
    const std::string comment = "#";
    std::string line = "";
    while (line == "") {
        std::getline(istr, line);
        auto found = line.find(comment);
        if (found != std::string::npos)
            line = line.substr(0, found);
        line = trim(line);
    }
    return line;
}

std::string trim(const std::string& line) {
    const std::string whitespace = " \t\r\n";
    auto first = line.find_first_not_of(whitespace);
    if (first == std::string::npos)
        return "";
    auto last = line.find_last_not_of(whitespace);
    auto range = last - first + 1;
    return line.substr(first, range);
}