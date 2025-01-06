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
    void delay(std::size_t ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
    void toggle_window_mode() {
        _fullscreen = !_fullscreen;
        if (_fullscreen) {
            auto mode = glfwGetVideoMode(_monitors[_monitor_i]);
            glfwSetWindowMonitor(_glfw_window_p, _monitors[_monitor_i], 0, 0, mode->width, mode->height, 0);
        }
        else {
            glfwSetWindowMonitor(_glfw_window_p, nullptr, 0, 0, _windowed_resolution.width, _windowed_resolution.height, 0);
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
    bool _fullscreen = false;
};