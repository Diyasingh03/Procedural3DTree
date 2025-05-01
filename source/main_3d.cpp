#define USE_MULTITHREADING false

// STL includes
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <functional>

// SDL includes
#include <SDL2/SDL.h>

// Application includes
#include "opengl/window.h"
#include "opengl/camera.h"
#include "opengl/mesh.h"
#include "opengl/texture.h"
#include "opengl/program.h"
#include "opengl/screenshot.h"
#include "opengl/grid.h"
#include "opengl/canvas.h"
#include "opengl/shadermanager.h"
#include "core/application.h"
#include "core/randomization.h"
#include "core/utilities.h"
#include "core/input.h"

#include "generation/turtle3d.h"
#include "generation/lsystem.h"
#include "generation/fractals.h"

#include "tree.h"

#include "thirdparty/imgui/imgui_impl.h"
#include "thirdparty/imgui/imgui.h"

/*
	Program configurations
*/
static const bool WINDOW_VSYNC = true;
static const int WINDOW_FULLSCREEN = 0;
static const int WINDOW_WIDTH = 1280;
static const int WINDOW_HEIGHT = 720;
static const float CAMERA_FOV = 60.0f;
static const float WINDOW_RATIO = WINDOW_WIDTH / float(WINDOW_HEIGHT);
static const int FPS_LIMIT = 0;

namespace fs = std::filesystem;

/*
	Application
*/
int main()
{
	fs::path contentFolder = fs::current_path().parent_path() / "content";
	InitializeApplication(ApplicationSettings{
		WINDOW_VSYNC, WINDOW_FULLSCREEN, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_RATIO, contentFolder
	});

	UniformRandomGenerator uniformGenerator;

	OpenGLWindow windowObj;
	windowObj.SetTitle("Tree Generation");
	windowObj.SetClearColor(0.5f, 0.5f, 0.5f, 1.0f);

	// Initialize ImGui after window is created
	SDL_Window* window = SDL_GL_GetCurrentWindow();
	SDL_GLContext gl_context = SDL_GL_GetCurrentContext();
	ImGuiImpl::Init(window, gl_context);

	GLuint defaultVao = 0;
	glGenVertexArrays(1, &defaultVao);
	glBindVertexArray(defaultVao);

printf(R"(
====================================================================
	
    L-system Tree Generator.

    Controls:
        Mouse controls the camera. (L: Rotate, M: Move, R: Zoom)

        6:              Toggle display of skeleton
        F:              Re-center camera on origin

        S:              Take screenshot

        G:              Generate new tree with current settings
        T:              Toggle between default and slimmer tree style
        Up arrow:       Increase L-system iterations (bigger tree)
        Down arrow:     Decrease L-system iterations (smaller tree)
        Left arrow:     Decrease branch divisions
        Right arrow:    Increase branch divisions

        Color Controls:
        1:              Increase leaf hue
        2:              Decrease leaf hue
        3:              Increase bark hue
        4:              Decrease bark hue

        ESC:            Close the application

    Please note that iterations greater than 6 takes a long time.
    The application will not refresh during generations and will
    appear to "hang".

====================================================================
)");

	/*
		Setup scene and controls
	*/
	Camera camera;
	camera.fieldOfView = CAMERA_FOV;

	GLQuad backgroundQuad;
	GLGrid grid;
	grid.size = 20.0f;
	grid.gridSpacing = 0.5f;

	TurntableController turntable(camera);
	turntable.position = glm::vec3{0.0f, 7.0f, 0.0f};
	turntable.sensitivity = 0.25f;
	turntable.Set(-25.0f, 15.0f, 15.0f);


	/*
		Load and initialize shaders
	*/
	GLTexture defaultTexture{contentFolder / "default.png"};
	defaultTexture.UseForDrawing();

	GLProgram defaultShader, lineShader, treeShader, leafShader, phongShader, backgroundShader, flowerShader;
	ShaderManager shaderManager;
	shaderManager.InitializeFolder(contentFolder);
	shaderManager.LoadShader(defaultShader, L"basic_vertex.glsl", L"basic_fragment.glsl");
	shaderManager.LoadShader(leafShader, L"leaf_vertex.glsl", L"leaf_fragment.glsl");
	shaderManager.LoadShader(phongShader, L"phong_vertex.glsl", L"phong_fragment.glsl");
	shaderManager.LoadShader(treeShader, L"phong_vertex.glsl", L"tree_fragment.glsl");
	shaderManager.LoadShader(lineShader, L"line_vertex.glsl", L"line_fragment.glsl");
	shaderManager.LoadShader(backgroundShader, L"background_vertex.glsl", L"background_fragment.glsl");
	shaderManager.LoadShader(flowerShader, L"flower_vertex.glsl", L"flower_fragment.glsl");

	// Initialize light source in shaders
	glm::vec4 lightColor{ 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec3 lightPosition{ 999999.0f };
	phongShader.Use(); 
		phongShader.SetUniformVec4("lightColor", lightColor);
		phongShader.SetUniformVec3("lightPosition", lightPosition);
	treeShader.Use(); 
		treeShader.SetUniformVec4("lightColor", lightColor);
		treeShader.SetUniformVec3("lightPosition", lightPosition);
	leafShader.Use(); 
		leafShader.SetUniformVec4("lightColor", lightColor);
		leafShader.SetUniformVec3("lightPosition", lightPosition);
	flowerShader.Use();
		flowerShader.SetUniformVec3("lightPosition", lightPosition);


	/*
		Build leaf texture and mesh
	*/
	GLTriangleMesh leafMesh;
	Canvas2D leafCanvas{128, 128};
	GenerateLeaf(leafCanvas, leafMesh);

	/*
		Build flower texture and mesh
	*/
	GLTriangleMesh flowerMesh;
	Canvas2D flowerCanvas{128, 128};
	GenerateFlower(flowerCanvas, flowerMesh);

	/*
		Build tree mesh
	*/
	GLLine skeletonLines, coordinateReferenceLines;
	GLTriangleMesh branchMeshes, crownLeavesMeshes, crownFlowersMeshes;
	bool showFlowers = true;
	auto GenerateRandomTree = [&](int iterations = 5, int subdivisions = 3) {
		printf("\r\nGenerating tree (%d iterations, %d subdivisions)... ", iterations, subdivisions);
		GenerateNewTree(skeletonLines, branchMeshes, crownLeavesMeshes, crownFlowersMeshes, leafMesh, flowerMesh, uniformGenerator, iterations, subdivisions, showFlowers);
	};
	GenerateRandomTree();

	/*
		Coordinate system reference lines
	*/
	glm::fvec3 orientationReferencePoint = glm::fvec3{ 0.0f, 0.0f, 0.0f };
	glm::fvec3 x = glm::fvec3{ 1.0f, 0.0f, 0.0f };
	glm::fvec3 y = glm::fvec3{ 0.0f, 1.0f, 0.0f };
	glm::fvec3 z = glm::fvec3{ 0.0f, 0.0f, 1.0f };
	coordinateReferenceLines.AddLine(orientationReferencePoint, orientationReferencePoint + x, glm::fvec4(x, 1.0f));
	coordinateReferenceLines.AddLine(orientationReferencePoint, orientationReferencePoint + y, glm::fvec4(y, 1.0f));
	coordinateReferenceLines.AddLine(orientationReferencePoint, orientationReferencePoint + z, glm::fvec4(z, 1.0f));
	coordinateReferenceLines.SendToGPU();

	/*
		User interaction options
	*/
	bool renderSkeleton = false;
	int treeIterations = 5;
	int treeSubdivisions = 3;

	// HSV to RGB conversion function
	auto hsvToRgb = [](float h, float s, float v) -> glm::vec3 {
		float c = v * s;
		float x = c * (1.0f - std::abs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
		float m = v - c;

		glm::vec3 rgb;
		if (h < 1.0f/6.0f)      rgb = glm::vec3(c, x, 0);
		else if (h < 2.0f/6.0f) rgb = glm::vec3(x, c, 0);
		else if (h < 3.0f/6.0f) rgb = glm::vec3(0, c, x);
		else if (h < 4.0f/6.0f) rgb = glm::vec3(0, x, c);
		else if (h < 5.0f/6.0f) rgb = glm::vec3(x, 0, c);
		else                    rgb = glm::vec3(c, 0, x);

		return rgb + glm::vec3(m);
	};

	// Color control variables
	glm::vec3 leafColor = glm::vec3(0.0f, 0.8f, 0.0f);  // Default green
	glm::vec3 barkColor = glm::vec3(0.4f, 0.2f, 0.0f);  // Default brown
	glm::vec3 flowerColor = glm::vec3(1.0f, 0.0f, 0.0f);  // Default red
	float leafHue = 0.3f;  // Green hue
	float barkHue = 0.1f;  // Brown hue
	float flowerHue = 0.0f;  // Red hue

	// Update shader colors
	auto UpdateColors = [&]() {
		leafShader.Use();
		leafShader.SetUniformVec3("leafColor", leafColor);
		treeShader.Use();
		treeShader.SetUniformVec3("barkColor", barkColor);
		flowerShader.Use();
		flowerShader.SetUniformVec3("flowerColor", flowerColor);
	};
	UpdateColors();

	/*
		Main application loop
	*/
	bool quit = false;
	bool captureMouse = false;
	double lastUpdate = 0.0;
	double deltaTime = 0.0;
	double fpsDelta = (FPS_LIMIT == 0) ? 0.0 : (1.0 / FPS_LIMIT);
	while (!quit)
	{
		if (WINDOW_VSYNC || FPS_LIMIT == 0)
		{
			deltaTime = 1.0/60.0; // Approximate frame time for VSYNC
			lastUpdate = SDL_GetTicks() / 1000.0;
		}
		else
		{
			deltaTime = (SDL_GetTicks() / 1000.0) - lastUpdate;
			if (deltaTime < fpsDelta) continue;
			lastUpdate = SDL_GetTicks() / 1000.0;
		}

		windowObj.SetTitle("FPS: " + FpsString(deltaTime));

		// Start new ImGui frame
		ImGuiImpl::NewFrame(window);

		// Create ImGui window for color controls
		ImGui::Begin("Tree Color Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		
		// Leaf Color Controls
		ImGui::Text("Leaf Color");
		if (ImGui::ColorEdit3("Leaf RGB", &leafColor[0]))
		{
			UpdateColors();
		}
		if (ImGui::SliderFloat("Leaf Hue", &leafHue, 0.0f, 1.0f))
		{
			leafColor = hsvToRgb(leafHue, 1.0f, 0.8f);
			UpdateColors();
		}
		
		// Bark Color Controls
		ImGui::Separator();
		ImGui::Text("Bark Color");
		if (ImGui::ColorEdit3("Bark RGB", &barkColor[0]))
		{
			UpdateColors();
		}
		if (ImGui::SliderFloat("Bark Hue", &barkHue, 0.0f, 1.0f))
		{
			barkColor = hsvToRgb(barkHue, 1.0f, 0.4f);
			UpdateColors();
		}

		// Flower Controls
		ImGui::Separator();
		ImGui::Text("Flower Controls");
		if (ImGui::Checkbox("Show Flowers", &showFlowers))
		{
			// Regenerate tree when flowers are toggled
			GenerateRandomTree(treeIterations, treeSubdivisions);
		}
		if (showFlowers)
		{
			if (ImGui::ColorEdit3("Flower RGB", &flowerColor[0]))
			{
				UpdateColors();
			}
			if (ImGui::SliderFloat("Flower Hue", &flowerHue, 0.0f, 1.0f))
			{
				flowerColor = hsvToRgb(flowerHue, 1.0f, 1.0f);
				UpdateColors();
			}
		}
		
		ImGui::End();

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			// Let ImGui process the event
			ImGuiImpl::ProcessEvent(&event);

			// Always process quit and keyboard events regardless of ImGui
			if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN)
			{
				quit = (event.type == SDL_QUIT) || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE);
				if (quit) break;

				if (event.type == SDL_KEYDOWN)
				{
					auto key = event.key.keysym.sym;

					if		(key == SDLK_6) renderSkeleton = !renderSkeleton;
					else if (key == SDLK_s) TakeScreenshot("screenshot.png", WINDOW_WIDTH, WINDOW_HEIGHT);
					else if (key == SDLK_f) turntable.SnapToOrigin();
					else if (key == SDLK_UP)    ++treeIterations;
					else if (key == SDLK_DOWN)  treeIterations = (treeIterations <= 1) ? 1 : treeIterations - 1;
					else if (key == SDLK_LEFT)  treeSubdivisions = (treeSubdivisions <= 1) ? 1 : treeSubdivisions - 1;
					else if (key == SDLK_RIGHT) ++treeSubdivisions;

					switch (key)
					{
					case SDLK_g:case SDLK_UP:case SDLK_DOWN:case SDLK_LEFT:case SDLK_RIGHT:
					{
						GenerateRandomTree(treeIterations, treeSubdivisions);
					}
					default: { break; }
					}
				}
			}
			else if (event.type == SDL_MOUSEWHEEL)
			{
				if (event.wheel.y > 0) // Scroll up (zoom in)
				{
					camera.fieldOfView = glm::max(10.0f, camera.fieldOfView - 5.0f); // Decrease FOV
				}
				else if (event.wheel.y < 0) // Scroll down (zoom out)
				{
					camera.fieldOfView = glm::min(90.0f, camera.fieldOfView + 5.0f); // Increase FOV
				}
			}
			// Only process mouse events if ImGui didn't consume them
			else if (!ImGui::GetIO().WantCaptureMouse)
			{
				if (event.type == SDL_MOUSEBUTTONDOWN)
				{
					captureMouse = true;
					SDL_ShowCursor(0);
					SDL_SetRelativeMouseMode(SDL_TRUE);

					auto button = event.button.button;
					     if (button == SDL_BUTTON_LEFT)   turntable.inputState = TurntableInputState::Rotate;
					else if (button == SDL_BUTTON_MIDDLE) turntable.inputState = TurntableInputState::Translate;
					else if (button == SDL_BUTTON_RIGHT)  turntable.inputState = TurntableInputState::Zoom;
				}
				else if (event.type == SDL_MOUSEBUTTONUP)
				{
					captureMouse = false;
					SDL_ShowCursor(1);
					SDL_SetRelativeMouseMode(SDL_FALSE);
				}
				else if (event.type == SDL_MOUSEMOTION && captureMouse)
				{
					turntable.ApplyMouseInput(-event.motion.xrel, event.motion.yrel);
				}
			}
		}

		/*
			Render scene
		*/
		windowObj.Clear();

		// Background color gradient
		backgroundShader.Use();
		backgroundQuad.Draw();
		glClear(GL_DEPTH_BUFFER_BIT);
		
		// Determine scene render properties
		glm::mat4 projection = camera.ViewProjectionMatrix();
		glm::mat4 mvp = projection * branchMeshes.transform.ModelMatrix();

		// Render tree branches
		treeShader.Use();
		treeShader.SetUniformVec3("cameraPosition", camera.GetPosition());
		treeShader.UpdateMVP(mvp);
		defaultTexture.UseForDrawing();
		glUniform1i(glGetUniformLocation(treeShader.Id(), "textureSampler"), 0);  // Bind texture to unit 0
		branchMeshes.Draw();

		// Render leaves
		leafShader.Use();
		leafShader.SetUniformFloat("sssBacksideAmount", 0.75f);
		leafShader.SetUniformVec3("cameraPosition", camera.GetPosition());
		leafShader.UpdateMVP(mvp);
		leafCanvas.GetTexture()->UseForDrawing();
		glUniform1i(glGetUniformLocation(leafShader.Id(), "textureSampler"), 0);  // Bind texture to unit 0
		crownLeavesMeshes.Draw();

		// Render flowers if enabled
		if (showFlowers)
		{
			// Enable alpha blending for flowers
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			flowerShader.Use();
			flowerShader.SetUniformVec3("cameraPosition", camera.GetPosition());
			flowerShader.UpdateMVP(mvp);
			flowerCanvas.GetTexture()->UseForDrawing();
			glUniform1i(glGetUniformLocation(flowerShader.Id(), "textureSampler"), 0);
			crownFlowersMeshes.Draw();
			
			// Disable alpha blending after flowers
			glDisable(GL_BLEND);
		}

		// Grid
		grid.Draw(projection);
		
		// Skeleton and coordinate axes
		glClear(GL_DEPTH_BUFFER_BIT);
		lineShader.UpdateMVP(projection);
		lineShader.Use();
		coordinateReferenceLines.Draw();
		if (renderSkeleton)
		{
			skeletonLines.Draw();
		}

		// Render ImGui
		ImGui::Render();
		ImGuiImpl::RenderDrawData(ImGui::GetDrawData());

		windowObj.SwapFramebuffer();
	}

	// Cleanup ImGui
	ImGuiImpl::Shutdown();
	exit(0);
}
