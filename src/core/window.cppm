module;
#include <print>
#include <chrono>
#include <thread>
#include <string>
#include <cstring>
#include <cstdint>
#include <vulkan/vulkan_hpp_macros.hpp>

#if __has_include(<GLFW/glfw3.h>)
#   include <vulkan/vulkan_core.h>
#   include <GLFW/glfw3.h>
#endif

export module window;
import vulkan_hpp;
import input;

export struct Window {
    void init(unsigned int width, unsigned int height, std::string name);
    void destroy();

    bool is_open();
    bool is_focused();
    bool is_minimized();
    bool is_maximized();

    auto get_size() -> vk::Extent2D;

    void poll_events();
    void delay(std::size_t ms);
    void toggle_window_mode();
    void set_mouse_capture_mode(bool captured);
    
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

module: private;
#if __has_include(<GLFW/glfw3.h>)
void Window::init(unsigned int width, unsigned int height, std::string name){
    // dynamic dispatcher init 1/3
    vk::detail::defaultDispatchLoaderDynamic.init();

    // initialize GLFW
    glfwInitVulkanLoader(vk::detail::defaultDispatchLoaderDynamic.vkGetInstanceProcAddr);
    if (!glfwInit()) {
        std::println("Failed to initialize GLFW");
        exit(1);
    }
    glfwSetErrorCallback([](int error, const char* description) {
        std::println("GLFW error {}: {}", error, description);
    });

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

    // dynamic dispatcher init 2/3
    vk::detail::defaultDispatchLoaderDynamic.init(_instance);

    // build list of monitors and video modes of the first (primary) monitor
    _monitors = glfwGetMonitors(&_monitor_count);
    
    // create window for rendering
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);
    _glfw_window_p = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    _windowed_resolution = vk::Extent2D { width, height };
    if (_glfw_window_p == nullptr) {
        std::println("Failed to create window");
        exit(1);
    }

    // enable raw mouse motion
    if (glfwRawMouseMotionSupported()) glfwSetInputMode(_glfw_window_p, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    else std::println("Raw mouse motion not supported");
    // register callbacks
    glfwSetKeyCallback(_glfw_window_p, [](GLFWwindow*, int key, int, int action, int) {
        if (action == GLFW_PRESS) Input::register_key_press(key);
		else if (action == GLFW_RELEASE) Input::register_key_release(key);
    });
    glfwSetMouseButtonCallback(_glfw_window_p, [](GLFWwindow*, int button, int action, int) {
        if (action == GLFW_PRESS) Input::register_mouse_press(button);
		else if (action == GLFW_RELEASE) Input::register_mouse_release(button);
    });
    glfwSetCursorPosCallback(_glfw_window_p, [](GLFWwindow*, double xpos, double ypos) {
        Input::register_mouse_move(xpos, ypos);
    });
    // set window icon
    std::vector<unsigned char> icon_data(16 * 16 * 4, 0);
    GLFWimage icon { 16, 16, icon_data.data() };
    glfwSetWindowIcon(_glfw_window_p, 1, &icon);

    // create vulkan surface from window
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(_instance, _glfw_window_p, nullptr, &surface);
    _surface = static_cast<vk::SurfaceKHR>(surface);
}
void Window::destroy(){
    _instance.destroySurfaceKHR(_surface);
    _instance.destroy();
    glfwDestroyWindow(_glfw_window_p);
    glfwTerminate();
}

bool Window::is_open() {
    return !glfwWindowShouldClose(_glfw_window_p);
}
bool Window::is_focused() {
    return glfwGetWindowAttrib(_glfw_window_p, GLFW_FOCUSED);
}
bool Window::is_minimized() {
    return glfwGetWindowAttrib(_glfw_window_p, GLFW_ICONIFIED);
}
bool Window::is_maximized() {
    return glfwGetWindowAttrib(_glfw_window_p, GLFW_MAXIMIZED);
}

auto Window::get_size() -> vk::Extent2D {
    int width, height;
    glfwGetFramebufferSize(_glfw_window_p, &width, &height);
    return { (uint32_t)width, (uint32_t)height };
}
void Window::poll_events() {
    glfwPollEvents();
}
void Window::delay(std::size_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
void Window::toggle_window_mode() {
    if (is_maximized()) {
        glfwSetWindowAttrib(_glfw_window_p, GLFW_DECORATED, GLFW_TRUE);
        glfwRestoreWindow(_glfw_window_p);
    }
    else {
        glfwSetWindowAttrib(_glfw_window_p, GLFW_DECORATED, GLFW_FALSE);
        glfwMaximizeWindow(_glfw_window_p);
    }
}
void Window::set_mouse_capture_mode(bool captured) {
    glfwSetInputMode(_glfw_window_p, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    Input::register_mouse_capture(captured);
}
#endif // __has_include(<GLFW/glfw3.h>)