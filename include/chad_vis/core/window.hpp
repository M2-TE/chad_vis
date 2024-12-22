#pragma once
#include <print>
#include <cstdint>
#include <SFML/Window.hpp>
#include <SFML/Window/Event.hpp>
#include <vulkan/vulkan.hpp>

struct Window {
    void init(unsigned int width, unsigned int height, std::string name) {
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
#       ifdef VULKAN_VALIDATION_LAYERS
            std::string validation_layer = "VK_LAYER_KHRONOS_validation";
            // only request if validation layers are available
            auto layer_props = vk::enumerateInstanceLayerProperties();
            bool available = false;
            for (auto& layer: layer_props) {
                auto res = std::strcmp(layer.layerName, validation_layer.data());
                fmt::println("{}", std::string(layer.layerName));
                if (res) available = true;
            }
            if (available) layers.push_back(validation_layer.data());
            else fmt::println("Validation layers requested but not present");
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
        _window = { sf::VideoMode({width, height}, 32), name, sf::Style::Default };
        VkSurfaceKHR surface;
        if (!_window.createVulkanSurface(_instance, surface)) {
            std::println("Failed to create vulkan surface");
            exit(1);
        }
        _surface = static_cast<vk::SurfaceKHR>(surface);
    }
    void destroy() {
    }

    sf::Window _window;
    vk::Instance _instance;
    vk::SurfaceKHR _surface;
};