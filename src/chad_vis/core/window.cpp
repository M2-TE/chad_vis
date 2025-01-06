#include "chad_vis/core/window.hpp"
#include "chad_vis/core/input.hpp"

void static error_callback(int error, const char* description) {
    std::println("GLFW error {}:\n{}", error, description);
}
void Window::init(unsigned int width, unsigned int height, std::string name) {
    // initialize GLFW
    glfwInitVulkanLoader(VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr);
    if (!glfwInit()) {
        std::println("Failed to initialize GLFW");
        exit(1);
    }
    glfwSetErrorCallback(error_callback);

    // check availability of required vulkan instance extensions
    uint32_t extension_count;
    const char** extensions_required = glfwGetRequiredInstanceExtensions(&extension_count);
    auto extensions_available = vk::enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < extension_count; i++) {
        bool available = false;
        for (auto available_ext: extensions_available) {
            if (std::strcmp(extensions_required[i], available_ext.extensionName) == 0) {
                available = true;
                break;
            }
        }
        if (!available) {
            std::println("Required vulkan instance extension {} is not available", extensions_required[i]);
            exit(1);
        }
    }

    // optionally enable debug layers
    std::vector<const char*> layers;
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
        .enabledExtensionCount = extension_count,
        .ppEnabledExtensionNames = extensions_required,
    };
    _instance = vk::createInstance(info_instance);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(_instance);

    // build list of monitors and video modes of the first (primary) monitor
    _monitors = glfwGetMonitors(&_monitor_count);
    
    // create window for rendering
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
    _glfw_window_p = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    _windowed_resolution = { width, height };
    if (_glfw_window_p == nullptr) {
        std::println("Failed to create window");
        exit(1);
    }

    // enable raw mouse motion
    if (glfwRawMouseMotionSupported()) glfwSetInputMode(_glfw_window_p, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    else std::println("Raw mouse motion not supported");
    // register callbacks
    glfwSetKeyCallback(_glfw_window_p, Input::key_callback);
    glfwSetCursorPosCallback(_glfw_window_p, Input::mouse_position_callback);
    glfwSetMouseButtonCallback(_glfw_window_p, Input::mouse_button_callback);

    // create vulkan surface from window
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(_instance, _glfw_window_p, nullptr, &surface);
    _surface = static_cast<vk::SurfaceKHR>(surface);
}
void Window::destroy() {
    _instance.destroySurfaceKHR(_surface);
    _instance.destroy();
    glfwDestroyWindow(_glfw_window_p);
    glfwTerminate();
}