/**
 * @file Theme.qml
 * @brief Design token singleton for QuantumVerse 4D Simulator UI
 *
 * Session 2 commit 1: centralizes all visual tokens so downstream commits
 * can substitute them mechanically. Inert until referenced — adding this
 * file and declaring it in qmldir changes zero pixels.
 *
 * Reference style: custom dark (Blender/Houdini-adjacent), 8px grid,
 * Lucide 24x24 stroke icons at 2px weight.
 */

import QtQuick

pragma Singleton

QtObject {
    readonly property color bg: "#0a0a1a"
    readonly property color surface: "#12122a"
    readonly property color surfaceHover: "#1a1a35"
    readonly property color surfaceActive: "#252545"
    readonly property color border: "#252545"
    readonly property color textPrimary: "#e0e0f0"
    readonly property color textSecondary: "#7a7a9a"
    readonly property color textDisabled: "#4a4a6a"
    readonly property color accent: "#22d3ee"
    readonly property color accentDim: "#0e7490"
    readonly property color danger: "#ef4444"
    readonly property color statusOk: "#22c55e"
    readonly property color statusWarn: "#f59e0b"
    readonly property color statusCrit: "#ef4444"

    readonly property int spaceXs: 4
    readonly property int spaceSm: 8
    readonly property int spaceMd: 12
    readonly property int spaceLg: 16
    readonly property int spaceXl: 24

    readonly property int rowH: 40
    readonly property int btnH: 32
    readonly property int railBtn: 40
    readonly property int sliderH: 20

    readonly property int radiusSm: 4
    readonly property int radiusMd: 6
    readonly property int radiusLg: 8

    readonly property int fontXs: 11
    readonly property int fontSm: 12
    readonly property int fontMd: 13
    readonly property int fontLg: 14
    readonly property string fontMono: "JetBrains Mono, Consolas, monospace"

    readonly property int durationFast: 100
    readonly property int durationMd: 150
    readonly property var easing: Easing.OutQuad
}
