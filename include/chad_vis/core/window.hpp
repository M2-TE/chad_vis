#pragma once
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

struct Window {
    void init(unsigned int width, unsigned int height, std::string name);
    void destroy();
    auto size() -> vk::Extent2D {
        int width, height;
        glfwGetFramebufferSize(_glfw_window_p, &width, &height);
        return { (uint32_t)width, (uint32_t)height };
    }
    auto focused() -> bool {
        return glfwGetWindowAttrib(_glfw_window_p, GLFW_FOCUSED);
    }
    auto minimized() -> bool {
        return glfwGetWindowAttrib(_glfw_window_p, GLFW_ICONIFIED);
    }
    auto maximized() -> bool {
        return glfwGetWindowAttrib(_glfw_window_p, GLFW_MAXIMIZED);
    }
    void delay(std::size_t ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
    void toggle_window_mode() {
        if (maximized()) {
            glfwSetWindowAttrib(_glfw_window_p, GLFW_DECORATED, GLFW_TRUE);
            glfwRestoreWindow(_glfw_window_p);
        }
        else {
            glfwSetWindowAttrib(_glfw_window_p, GLFW_DECORATED, GLFW_FALSE);
            glfwMaximizeWindow(_glfw_window_p);
        }
    }
    
    vk::Instance _instance;
    vk::SurfaceKHR _surface;
    GLFWwindow* _glfw_window_p;
    // monitors
    GLFWmonitor** _monitors;
    int _monitor_count;
    int _monitor_i = 0;
    // window state
    vk::Extent2D _windowed_resolution;
};