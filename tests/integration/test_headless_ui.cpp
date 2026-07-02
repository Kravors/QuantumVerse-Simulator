#include "glad.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#include "implot.h"
#include <GLFW/glfw3.h>
#include "ui4d/ImGuiBackend.h"
#include <cstdio>
#include <cassert>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("Headless UI Test: Starting...\n");
    
    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    });

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "HeadlessTest", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    printf("Headless UI Test: OpenGL initialized\n");

    ui4d::ImGuiBackend& imgui = ui4d::ImGuiBackend::getInstance();
    if (!imgui.initialize(window)) {
        fprintf(stderr, "Failed to initialize ImGui\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    printf("Headless UI Test: ImGui initialized\n");

    ImPlot::CreateContext();
    imgui.enableDocking(true);
    imgui.enableViewports(true);

    printf("Headless UI Test: ImPlot context created\n");
    printf("Headless UI Test: PASSED\n");

    ImPlot::DestroyContext();
    imgui.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}