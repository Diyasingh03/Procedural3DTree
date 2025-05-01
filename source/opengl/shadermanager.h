#pragma once
#include "program.h"
#include <string>
#include <filesystem>

class ShaderManager
{
protected:
	std::filesystem::path rootFolder;

public:
	ShaderManager() = default;
	~ShaderManager() = default;

	void InitializeFolder(std::filesystem::path shaderFolder);
	void LoadShader(GLProgram& targetProgram, std::wstring vertexFilename, std::wstring fragmentFilename);
};