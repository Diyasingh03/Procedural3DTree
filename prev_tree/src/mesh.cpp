#define NOMINMAX
#include "mesh.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>

// Helper functions
int indexOfNumberLetter(std::string& str, int offset);
int lastIndexOfNumberLetter(std::string& str);
std::vector<std::string> split(const std::string &s, char delim);

// Constructor - load mesh from file
Mesh::Mesh(std::string filename, const ObjType mType, bool keepLocalGeometry) {
	minBB = glm::vec3(std::numeric_limits<float>::max());
	maxBB = glm::vec3(std::numeric_limits<float>::lowest());

	meshType = mType;

	vao = 0;
	vbuf = 0;
	vcount = 0;
	load(filename, keepLocalGeometry);
}

// Draw the mesh
void Mesh::draw() {
	if (vao) {
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, vcount, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
}

// Load a wavefront OBJ file
void Mesh::load(std::string filename, bool keepLocalGeometry) {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	
	// Temporary vectors for OBJ data
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	
	std::ifstream file(filename);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + filename);
	}

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string type;
		iss >> type;

		if (type == "v") {  // Vertex position
			glm::vec3 pos;
			iss >> pos.x >> pos.y >> pos.z;
			positions.push_back(pos);
		}
		else if (type == "vn") {  // Vertex normal
			glm::vec3 norm;
			iss >> norm.x >> norm.y >> norm.z;
			normals.push_back(norm);
		}
		else if (type == "vt") {  // Texture coordinate
			glm::vec2 uv;
			iss >> uv.x >> uv.y;
			uvs.push_back(uv);
		}
		else if (type == "f") {  // Face
			std::string vertex;
			std::vector<unsigned int> faceIndices;
			
			while (iss >> vertex) {
				std::istringstream vss(vertex);
				std::string index;
				std::vector<int> vertexIndices;
				
				while (std::getline(vss, index, '/')) {
					if (!index.empty()) {
						vertexIndices.push_back(std::stoi(index) - 1);
					}
					else {
						vertexIndices.push_back(-1);
					}
				}
				
				// Create vertex
				Vertex v;
				v.pos = positions[vertexIndices[0]];
				if (vertexIndices.size() > 1 && vertexIndices[1] != -1) {
					v.uv = uvs[vertexIndices[1]];
				}
				if (vertexIndices.size() > 2 && vertexIndices[2] != -1) {
					v.norm = normals[vertexIndices[2]];
				}
				
				// Add vertex and index
				vertices.push_back(v);
				indices.push_back(static_cast<unsigned int>(vertices.size() - 1));
			}
		}
	}

	// Calculate tangents and bitangents
	for (size_t i = 0; i < indices.size(); i += 3) {
		Vertex& v0 = vertices[indices[i]];
		Vertex& v1 = vertices[indices[i + 1]];
		Vertex& v2 = vertices[indices[i + 2]];

		glm::vec3 edge1 = v1.pos - v0.pos;
		glm::vec3 edge2 = v2.pos - v0.pos;
		glm::vec2 deltaUV1 = v1.uv - v0.uv;
		glm::vec2 deltaUV2 = v2.uv - v0.uv;

		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

		v0.tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		v0.tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		v0.tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

		v0.bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
		v0.bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
		v0.bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

		v1.tangent = v0.tangent;
		v1.bitangent = v0.bitangent;
		v2.tangent = v0.tangent;
		v2.bitangent = v0.bitangent;
	}

	// Create mesh
	create(vertices, indices);

	// Update bounding box
	for (const auto& vertex : vertices) {
		minBB = glm::min(minBB, vertex.pos);
		maxBB = glm::max(maxBB, vertex.pos);
	}

	// Clean up if local geometry not needed
	if (!keepLocalGeometry) {
		vertices.clear();
		vertices.shrink_to_fit();
	}
}

void Mesh::create(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
	// Release any existing resources
	release();

	// Store vertices for later use if needed
	this->vertices = vertices;
	vcount = static_cast<GLsizei>(indices.size());

	// Create and bind VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create and populate vertex buffer
	glGenBuffers(1, &vbuf);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	// Create and populate index buffer
	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	// Set up vertex attributes
	// Position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
	// Normal
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, norm));
	// UV
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	// Tangent
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
	// Bitangent
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

	// Calculate bounding box
	minBB = glm::vec3(std::numeric_limits<float>::max());
	maxBB = glm::vec3(std::numeric_limits<float>::lowest());
	for (const auto& vertex : vertices) {
		minBB = glm::min(minBB, vertex.pos);
		maxBB = glm::max(maxBB, vertex.pos);
	}

	// Unbind VAO
	glBindVertexArray(0);
}

// Release resources
void Mesh::release() {
	if (vao) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
	if (vbuf) {
		glDeleteBuffers(1, &vbuf);
		vbuf = 0;
	}
	vcount = 0;
	if (!vertices.empty()) {
		vertices.clear();
	}
}

int indexOfNumberLetter(std::string& str, int offset) {
	for (int i = offset; i < int(str.length()); ++i) {
		if ((str[i] >= '0' && str[i] <= '9') || str[i] == '-' || str[i] == '.') return i;
	}
	return (int)str.length();
}
int lastIndexOfNumberLetter(std::string& str) {
	for (int i = int(str.length()) - 1; i >= 0; --i) {
		if ((str[i] >= '0' && str[i] <= '9') || str[i] == '-' || str[i] == '.') return i;
	}
	return 0;
}
std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;

	std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }

    return elems;
}

Mesh::Mesh() : VAO(0), VBO(0), EBO(0) {}

Mesh::~Mesh() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

void Mesh::setupMesh() {
    // Generate buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    // Bind VAO
    glBindVertexArray(VAO);
    
    // Bind and fill VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    
    // Bind and fill EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    
    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    
    // Texture coordinate attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    
    // Tangent attribute
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    
    // Bitangent attribute
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
    
    // Unbind
    glBindVertexArray(0);
}

void Mesh::draw(Shader& shader, bool isLeaf) {
    // Set uniforms
    shader.setBool("isLeaf", isLeaf);
    
    // Bind textures
    texture.bind(0);
    normalMap.bind(1);
    
    // Draw mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Unbind textures
    texture.unbind();
    normalMap.unbind();
}
