#include "imgui_impl.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

namespace ImGuiImpl
{
    bool Init(SDL_Window* window, SDL_GLContext gl_context)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
        ImGui_ImplOpenGL3_Init("#version 130");

        return true;
    }

    void Shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    void NewFrame(SDL_Window* window)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
    }

    void RenderDrawData(ImDrawData* draw_data)
    {
        ImGui_ImplOpenGL3_RenderDrawData(draw_data);
    }

    bool ProcessEvent(SDL_Event* event)
    {
        return ImGui_ImplSDL2_ProcessEvent(event);
    }
} 