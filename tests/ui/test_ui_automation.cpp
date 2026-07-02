#include "glad.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#include "implot.h"
#include <GLFW/glfw3.h>
#include "ui4d/ImGuiBackend.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static unsigned char* grabFramebuffer(int w, int h) {
    auto* pixels = new unsigned char[w * h * 3];
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    return pixels;
}

static double perceptualHash(unsigned char* data, int w, int h) {
    int size = (w / 8) * (h / 8);
    double hash = 0.0;
    for (int i = 0; i < size; i++) {
        hash += data[i * 8 + i * 8] / 255.0;
    }
    return hash / size;
}

int main(int argc, char** argv) {
    bool captureScreenshot = false;
    const char* outputPath = "test_screenshot.png";
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            captureScreenshot = true;
            outputPath = argv[i + 1];
        }
    }

    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    });

    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "UI Test", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    ui4d::ImGuiBackend& imgui = ui4d::ImGuiBackend::getInstance();
    if (!imgui.initialize(window)) {
        fprintf(stderr, "Failed to initialize ImGui\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    ImPlot::CreateContext();
    imgui.enableDocking(true);
    imgui.enableViewports(true);

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    imgui.newFrame();
    ImGui::Begin("Test Window");
    ImGui::Text("UI Automation Test");
    ImGui::End();
    imgui.render();

    if (captureScreenshot) {
        unsigned char* pixels = grabFramebuffer(display_w, display_h);
        stbi_write_png(outputPath, display_w, display_h, 3, pixels, display_w * 3);
        delete[] pixels;
        printf("Screenshot saved: %s\n", outputPath);
    }

    ImPlot::DestroyContext();
    imgui.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}