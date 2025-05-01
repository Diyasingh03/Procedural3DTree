#include "shadermanager.h"
#include "../core/utilities.h"

namespace fs = std::filesystem;

void ShaderManager::InitializeFolder(std::filesystem::path shaderFolder)
{
	rootFolder = shaderFolder;
}

void ShaderManager::LoadShader(GLProgram& targetProgram, std::wstring vertexFilename, std::wstring fragmentFilename)
{
	std::string fragment, vertex;
	if (!LoadText(rootFolder/vertexFilename, vertex))
	{
		wprintf(L"\r\nFailed to read shader: %Ls\r\n", vertexFilename.c_str());
		return;
	}

	if (!LoadText(rootFolder/fragmentFilename, fragment))
	{
		wprintf(L"\r\nFailed to read shader: %Ls\r\n", fragmentFilename.c_str());
		return;
	}

	targetProgram.LoadFragmentShader(fragment);
	targetProgram.LoadVertexShader(vertex);
	targetProgram.CompileAndLink();
}
