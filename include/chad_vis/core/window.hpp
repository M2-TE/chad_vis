#pragma once
#include <SFML/Window.hpp>
#include <vulkan/vulkan.hpp>

struct Window {
    void init(unsigned int width, unsigned int height, std::string name);
    void destroy();
    auto size() -> vk::Extent2D {
        auto size = _sfml_window.getSize();
        return { size.x, size.y };
    }
    void delay(std::size_t ms) {
        sf::sleep(sf::milliseconds(ms));
    }
    
    sf::WindowBase _sfml_window;
    vk::Instance _instance;
    vk::SurfaceKHR _surface;
};