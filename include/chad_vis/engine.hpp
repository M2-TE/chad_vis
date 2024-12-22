#pragma once
#include <print>
#include <chad_vis/window.hpp>

struct Engine {
    void init_instance() {

    }
    void init() {
        // dynamic dispatcher init 1/3
        VULKAN_HPP_DEFAULT_DISPATCHER.init();
        _instance = _window.init(1280, 720, "CHAD Visualizer");
    }
    void destroy() {
        std::println("bye bye");
    }
    void run() {}

    Window _window;
    vk::Instance _instance;
};