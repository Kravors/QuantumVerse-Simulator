#pragma once

#include "glad.h"
#include "imgui.h"
#include <string>

struct GLFWwindow;

namespace ui4d {

enum class ThemeType {
    Dark,
    Light,
    HighContrast
};

class ImGuiBackend {
public:
    ImGuiBackend();
    ~ImGuiBackend();

    bool initialize(GLFWwindow* window);
    void shutdown();

    void newFrame();
    void render();

    void setTheme(ThemeType theme);
    void setThemeColor(ImGuiCol idx, const ImVec4& color);

    void enableDocking(bool enable = true);
    void enableViewports(bool enable = true);

    float getDPIScale() const;
    void setDPIScale(float scale);

    static ImGuiBackend& getInstance();

private:
    static ImGuiBackend s_instance;
    bool m_initialized;
    float m_dpiScale;
    ThemeType m_currentTheme;

    void applyTheme(ThemeType theme);
    void setupFonts(float scale);
};

}