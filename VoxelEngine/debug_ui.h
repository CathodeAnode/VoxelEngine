#ifndef __VE_DEBUG_UI_H_
#define __VE_DEBUG_UI_H_

#include <string>
#include <vector>
#include <format>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

struct GLFWwindow;

class DebugUI
{
public:
    static void Init(GLFWwindow* window);
    static void Shutdown();

    static void BeginFrame();
    static void EndFrame();

    static void Toggle(bool enabled);
    static bool IsEnabled();

    template<typename... Args>
    static void Print(const std::string& fmt, Args&&... args)
    {
        ImGui::Text(fmt.c_str(), args...);
    }

private:
    static inline bool m_Enabled = true;
};

#endif
