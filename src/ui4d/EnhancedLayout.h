#pragma once

#include <string>
#include <array>

namespace quantumverse {
namespace ui4d {

/**
 * @brief Enhanced UI layout configuration for 4D visualization
 * 
 * Provides advanced layout capabilities including:
 * - Responsive multi-panel design
 * - Theme support (dark/light/space modes)
 * - Animation transitions
 * - Custom widget styling
 */
class EnhancedLayout {
public:
    enum class Theme {
        DARK,
        LIGHT,
        SPACE
    };

    struct ColorScheme {
        std::array<float, 4> background;
        std::array<float, 4> panelBackground;
        std::array<float, 4> accent;
        std::array<float, 4> text;
        std::array<float, 4> textSecondary;
        std::array<float, 4> highlight;
        std::array<float, 4> warning;
        std::array<float, 4> error;
    };

    EnhancedLayout();

    /**
     * @brief Get current theme
     */
    Theme getTheme() const { return m_theme; }

    /**
     * @brief Set theme
     */
    void setTheme(Theme theme) { m_theme = theme; }

    /**
     * @brief Get sidebar width
     */
    float getSidebarWidth() const { return m_sidebarWidth; }

    /**
     * @brief Set sidebar width
     */
    void setSidebarWidth(float width) { m_sidebarWidth = width; }

    /**
     * @brief Get panel opacity
     */
    float getPanelOpacity() const { return m_panelOpacity; }

    /**
     * @brief Set panel opacity
     */
    void setPanelOpacity(float opacity) { m_panelOpacity = opacity; }

    /**
     * @brief Get color scheme for current theme
     */
    ColorScheme currentColorScheme() const;

    /**
     * @brief Get predefined color schemes
     */
    static ColorScheme getDarkScheme();
    static ColorScheme getLightScheme();
    static ColorScheme getSpaceScheme();

private:
    Theme m_theme;
    float m_sidebarWidth;
    float m_panelOpacity;
};

} // namespace ui4d
} // namespace quantumverse