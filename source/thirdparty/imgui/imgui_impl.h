#pragma once
#include "imgui.h"
#include <SDL2/SDL.h>
#include <glad/glad.h>

namespace ImGuiImpl
{
    bool Init(SDL_Window* window, SDL_GLContext gl_context);
    void Shutdown();
    void NewFrame(SDL_Window* window);
    void RenderDrawData(ImDrawData* draw_data);
    bool ProcessEvent(SDL_Event* event);
} 