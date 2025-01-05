#include "chad_vis/core/window.hpp"

void Window::init(unsigned int width, unsigned int height, std::string name) {
    // check availability of required vulkan instance extensions
    auto extensions_required = sf::Vulkan::getGraphicsRequiredInstanceExtensions();
    auto extensions_available = vk::enumerateInstanceExtensionProperties();
    uint32_t extensions_count = 0;
    for (auto available: extensions_available) {
        for (auto required: extensions_required) {
            if (std::strcmp(available.extensionName, required) == 0) {
                extensions_count++;
                break;
            }
        }
    }
    if (extensions_count < extensions_required.size()) {
        std::println("Required vulkan instance extensions are not available");
        exit(1);
    }

    // optionally enable debug layers
    std::vector<const char*> layers;
#       ifdef VULKAN_ENABLE_VALIDATION_LAYERS
        std::string validation_layer = "VK_LAYER_KHRONOS_validation";
        // only request if validation layers are available
        auto layer_props = vk::enumerateInstanceLayerProperties();
        bool available = false;
        for (auto& layer: layer_props) {
            auto res = std::strcmp(layer.layerName, validation_layer.data());
            if (res) available = true;
        }
        if (available) layers.push_back(validation_layer.data());
        else std::println("Validation layers requested but not present");
#       endif

    // create vulkan instance
    vk::ApplicationInfo info_app {
        .pApplicationName = name.c_str(),
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName = "TBD",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion = vk::ApiVersion13
    };
    vk::InstanceCreateInfo info_instance {
        .pApplicationInfo = &info_app,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions_required.size()),
        .ppEnabledExtensionNames = extensions_required.data()
    };
    _instance = vk::createInstance(info_instance);

    // create actual window
    // auto fullscreen_modes = sf::VideoMode::getFullscreenModes();
    // for (auto mode: fullscreen_modes) {
    //     std::println("Fullscreen mode: {}x{}@{}bpp", mode.size.x, mode.size.y, mode.bitsPerPixel);
    // }
    _sfml_window.create(sf::VideoMode({width, height}, 32), name, sf::Style::Default, sf::State::Windowed);
    
    // set icon to be invisible
    std::vector<std::uint8_t> pixels(16 * 16 * 4, 0);
    _sfml_window.setIcon({16, 16}, pixels.data());

    // create vulkan surface from window
    VkSurfaceKHR surface;
    if (!_sfml_window.createVulkanSurface(_instance, surface)) {
        std::println("Failed to create vulkan surface");
        exit(1);
    }
    _surface = static_cast<vk::SurfaceKHR>(surface);
}
void Window::destroy() {
}