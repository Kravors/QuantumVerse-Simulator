#include "ImGuiBackend.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <imgui.h>

namespace ui4d {

ImGuiBackend ImGuiBackend::s_instance;

ImGuiBackend::ImGuiBackend()
    : m_initialized(false)
    , m_dpiScale(1.0f)
    , m_currentTheme(ThemeType::Dark)
{
}

ImGuiBackend::~ImGuiBackend() {
    shutdown();
}

bool ImGuiBackend::initialize(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    applyTheme(ThemeType::Dark);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    m_initialized = true;
    return true;
}

void ImGuiBackend::shutdown() {
    if (m_initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(nullptr);
        m_initialized = false;
    }
}

void ImGuiBackend::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiBackend::render() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiBackend::setTheme(ThemeType theme) {
    if (m_initialized) {
        applyTheme(theme);
        m_currentTheme = theme;
    }
}

void ImGuiBackend::setThemeColor(ImGuiCol idx, const ImVec4& color) {
    ImGui::GetStyle().Colors[idx] = color;
}

void ImGuiBackend::enableDocking(bool enable) {
    ImGuiIO& io = ImGui::GetIO();
    if (enable) {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    } else {
        io.ConfigFlags &= ~ImGuiConfigFlags_DockingEnable;
    }
}

void ImGuiBackend::enableViewports(bool enable) {
    ImGuiIO& io = ImGui::GetIO();
    if (enable) {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    } else {
        io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
    }
}

float ImGuiBackend::getDPIScale() const {
    return m_dpiScale;
}

void ImGuiBackend::setDPIScale(float scale) {
    m_dpiScale = scale;
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = scale;
}

ImGuiBackend& ImGuiBackend::getInstance() {
    return s_instance;
}

void ImGuiBackend::applyTheme(ThemeType theme) {
    ImGuiStyle& style = ImGui::GetStyle();

    switch (theme) {
        case ThemeType::Dark:
            ImGui::StyleColorsDark();
            break;
        case ThemeType::Light:
            ImGui::StyleColorsLight();
            break;
        case ThemeType::HighContrast: {
            style = ImGuiStyle{};
            style.WindowRounding = 0.0f;
            style.GrabRounding = 0.0f;
            style.FrameRounding = 0.0f;
            
            ImVec4* colors = style.Colors;
            colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            colors[ImGuiCol_TextDisabled] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_PopupBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
            colors[ImGuiCol_Border] = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
            colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.0f, 0.0f, 0.5f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.3f, 0.0f, 0.0f, 0.7f);
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.0f, 0.0f, 0.5f);
            colors[ImGuiCol_TitleBg] = ImVec4(0.8f, 0.1f, 0.1f, 1.0f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
            colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.4f, 0.0f, 0.0f, 0.5f);
            colors[ImGuiCol_MenuBarBg] = ImVec4(0.2f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
            colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.8f, 0.1f, 0.1f, 1.0f);
            colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
            colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            colors[ImGuiCol_Button] = ImVec4(0.8f, 0.1f, 0.1f, 1.0f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
            colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            colors[ImGuiCol_PlotLines] = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
            colors[ImGuiCol_PlotHistogram] = ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
            colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.9f, 0.4f, 0.4f, 1.0f);
            colors[ImGuiCol_TableHeaderBg] = ImVec4(0.2f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_TableBorderStrong] = ImVec4(0.8f, 0.1f, 0.1f, 1.0f);
            colors[ImGuiCol_TableBorderLight] = ImVec4(0.6f, 0.1f, 0.1f, 1.0f);
            colors[ImGuiCol_Tab] = ImVec4(0.3f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_TabActive] = ImVec4(0.5f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_TabHovered] = ImVec4(0.4f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_TabDimmed] = ImVec4(0.2f, 0.0f, 0.0f, 1.0f);
            break;
        }
    }
}

}