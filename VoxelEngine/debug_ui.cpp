#include "debug_ui.h"

void DebugUI::Init(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Backend init (example: GLFW + OpenGL3)
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
}

void DebugUI::Shutdown() 
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void DebugUI::Toggle(bool e)
{
    m_Enabled = e;
}

bool DebugUI::IsEnabled()
{
    return m_Enabled;
}

int DebugUI::AddPlotSeries(const std::string& name, size_t maxPoints)
{
    m_Plots.push_back({ name, {}, maxPoints });
    return static_cast<int>(m_Plots.size() - 1);
}

void DebugUI::AddPlot(int seriesId, float value)
{
    if (seriesId < 0 || seriesId >= (int)m_Plots.size())
        return;

    auto& ps = m_Plots[seriesId];

    if (ps.values.size() >= ps.maxPoints)
        ps.values.erase(ps.values.begin());

    ps.values.push_back(value);
}

void DebugUI::BeginFrame()
{
    if (!m_Enabled)
        return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug Overlay", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav);

    // Position top-left with small padding
    ImGui::SetWindowPos(ImVec2(10, 10), ImGuiCond_Always);
}

void DebugUI::EndFrame()
{
    if (!m_Enabled)
        return;

    for(const auto& text: m_Text)
        ImGui::TextUnformatted(text.c_str());

    _RenderPlots();

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    m_Text.clear();
}

void DebugUI::_AddText(const std::string& s)
{
    m_Text.push_back(s);
}

void DebugUI::_RenderPlots()
{
    for (auto& ps : m_Plots)
    {
        if (!ps.values.empty())
        {
            ImGui::PlotLines(
                ps.name.c_str(),
                ps.values.data(),
                static_cast<int>(ps.values.size()),
                0,
                nullptr,
                FLT_MAX,
                FLT_MAX,
                ImVec2(0, 80)
            );
        }
    }
}