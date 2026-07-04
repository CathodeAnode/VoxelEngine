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
        _AddText(std::vformat(fmt, std::make_format_args(args...)));
    }

private:
    static void _AddText(const std::string& s);

private:
    static inline bool m_Enabled = true;
    static inline std::vector<std::string> m_Text;
};

#endif
