/**
 * @file EnhancedLayout.cpp
 * @brief Implementation of enhanced UI layout configuration
 */

#include "EnhancedLayout.h"
#include <algorithm>

namespace quantumverse {
namespace ui4d {

EnhancedLayout::EnhancedLayout()
    : m_theme(Theme::SPACE)
    , m_sidebarWidth(280.0f)
    , m_panelOpacity(0.85f)
{
}

EnhancedLayout::ColorScheme EnhancedLayout::currentColorScheme() const {
    switch (m_theme) {
        case Theme::DARK: return getDarkScheme();
        case Theme::LIGHT: return getLightScheme();
        case Theme::SPACE: return getSpaceScheme();
        default: return getSpaceScheme();
    }
}

EnhancedLayout::ColorScheme EnhancedLayout::getDarkScheme() {
    return ColorScheme{
        {0.1f, 0.1f, 0.12f, 1.0f},     // background
        {0.15f, 0.15f, 0.18f, 0.85f}, // panelBackground
        {0.2f, 0.6f, 1.0f, 1.0f},     // accent
        {0.9f, 0.9f, 0.9f, 1.0f},      // text
        {0.6f, 0.6f, 0.6f, 1.0f},      // textSecondary
        {0.3f, 0.7f, 1.0f, 1.0f},     // highlight
        {1.0f, 0.7f, 0.2f, 1.0f},     // warning
        {1.0f, 0.3f, 0.3f, 1.0f}       // error
    };
}

EnhancedLayout::ColorScheme EnhancedLayout::getLightScheme() {
    return ColorScheme{
        {0.95f, 0.95f, 0.97f, 1.0f},  // background
        {0.98f, 0.98f, 1.0f, 0.85f},  // panelBackground
        {0.1f, 0.4f, 0.8f, 1.0f},      // accent
        {0.1f, 0.1f, 0.1f, 1.0f},      // text
        {0.4f, 0.4f, 0.4f, 1.0f},      // textSecondary
        {0.1f, 0.5f, 0.9f, 1.0f},      // highlight
        {0.9f, 0.5f, 0.1f, 1.0f},      // warning
        {0.8f, 0.2f, 0.2f, 1.0f}       // error
    };
}

EnhancedLayout::ColorScheme EnhancedLayout::getSpaceScheme() {
    return ColorScheme{
        {0.04f, 0.04f, 0.1f, 1.0f},     // background (deep space blue)
        {0.08f, 0.08f, 0.16f, 0.85f},  // panelBackground
        {0.4f, 0.8f, 1.0f, 1.0f},      // accent (cyan)
        {0.9f, 0.9f, 0.95f, 1.0f},     // text
        {0.7f, 0.7f, 0.8f, 1.0f},      // textSecondary
        {0.5f, 0.9f, 1.0f, 1.0f},      // highlight
        {1.0f, 0.8f, 0.3f, 1.0f},      // warning (amber)
        {1.0f, 0.4f, 0.4f, 1.0f}      // error (red)
    };
}

} // namespace ui4d
} // namespace quantumverse