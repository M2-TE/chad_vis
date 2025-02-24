module;
#include <print>
#include <string>
#include <cstring>
#include <cstdint>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_hpp_macros.hpp>
export module window;
import vulkan_hpp;
import input;

export struct Window {
    struct CreateInfo;
    enum Mode { eFullscreen, eBorderless, eWindowed };
    void init(const CreateInfo& info);
    void destroy();
    void handle_event(void* event);

    void delay(uint32_t ms);
    void set_window_mode(Mode window_mode);
    void set_mouse_relative(bool relative);
    
    vk::Instance _instance;
    vk::SurfaceKHR _surface;
    SDL_Window* _sdl_window_p;
    // window state
    Mode _mode;
    Mode _fullscreen_mode;
    vk::Extent2D _size;
    SDL_DisplayMode _fullscreen_display_mode;
    bool _focused;
    bool _minimized; // TODO
};
struct Window::CreateInfo {
    std::string name;
    vk::Extent2D size = { 1280, 720 };
    Window::Mode window_mode = Window::eWindowed;
    Window::Mode fullscreen_mode = Window::eBorderless;
};

module: private;
void Window::init(const CreateInfo& info) {
    // init only the video subsystem
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland");
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) std::println("{}", SDL_GetError());

    // dynamic dispatcher init 1/3
    vk::detail::defaultDispatchLoaderDynamic.init();

    // get required extensions for vulkan instance
    Uint32 extension_count;
    const char* const* extensions_required = SDL_Vulkan_GetInstanceExtensions(&extension_count);
    if (extensions_required == nullptr) std::println("{}", SDL_GetError());
    // check availability of extensions
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
        .pApplicationName = info.name.c_str(),
        .applicationVersion = vk::makeApiVersion(0, 1, 0, 0),
        .pEngineName = "CHAD Visualizer",
        .engineVersion = vk::makeApiVersion(0, 1, 0, 0),
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

    // create SDL window and a corresponding Vulkan surface
    _sdl_window_p = SDL_CreateWindow(info.name.c_str(), info.size.width, info.size.height,
        SDL_WINDOW_VULKAN |
        SDL_WINDOW_RESIZABLE
    );
    if (_sdl_window_p == nullptr) std::println("{}", SDL_GetError());
    if (!SDL_Vulkan_CreateSurface(_sdl_window_p, _instance, nullptr, (VkSurfaceKHR*)&_surface)) std::println("{}", SDL_GetError());

    _size = info.size;
    _minimized = false;
    _focused = true;
    _mode = info.window_mode;
    _fullscreen_mode = info.fullscreen_mode;

    // pick the first display mode for the current display
    SDL_DisplayID display_id = SDL_GetDisplayForWindow(_sdl_window_p);
    if (display_id == 0) std::println("{}", SDL_GetError());
    int display_mode_count;
    SDL_DisplayMode** display_modes = SDL_GetFullscreenDisplayModes(display_id, &display_mode_count);
    if (display_modes == nullptr) std::println("{}", SDL_GetError());
    _fullscreen_display_mode = **display_modes;
    if (_mode == Mode::eFullscreen) SDL_SetWindowFullscreenMode(_sdl_window_p, &_fullscreen_display_mode);
}
void Window::destroy() {
    _instance.destroySurfaceKHR(_surface);
    _instance.destroy();
    SDL_DestroyWindow(_sdl_window_p);
}
void Window::handle_event(void* event_p) {
    SDL_Event& event = *static_cast<SDL_Event*>(event_p);
    switch (event.type) {
        case SDL_EventType::SDL_EVENT_KEY_DOWN: Input::register_key_press(event.key.key); break;
        case SDL_EventType::SDL_EVENT_KEY_UP: Input::register_key_release(event.key.key); break;
        case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_DOWN: Input::register_mouse_press(event.button.button); break;
        case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_UP: Input::register_mouse_release(event.button.button); break;
        case SDL_EventType::SDL_EVENT_MOUSE_MOTION: Input::register_mouse_delta(event.motion.xrel, event.motion.yrel); break;
        //
        case SDL_EventType::SDL_EVENT_WINDOW_MINIMIZED: _minimized = true; std::println("Minimized"); break;
        case SDL_EventType::SDL_EVENT_WINDOW_FOCUS_LOST: _focused = false; break;
        case SDL_EventType::SDL_EVENT_WINDOW_FOCUS_GAINED: _focused = true; break;
        case SDL_EventType::SDL_EVENT_WINDOW_MAXIMIZED: _mode = _fullscreen_mode; break;
        case SDL_EventType::SDL_EVENT_WINDOW_RESTORED: _mode = Mode::eWindowed; break;
        case SDL_EventType::SDL_EVENT_WINDOW_RESIZED: _size = { (uint32_t)event.window.data1, (uint32_t)event.window.data2 };
        default: break;
    }
}
void Window::delay(uint32_t ms) {
    SDL_Delay(ms);
}
void Window::set_window_mode(Mode window_mode) {
    switch (window_mode) {
        case Mode::eFullscreen: {
            SDL_SetWindowFullscreenMode(_sdl_window_p, &_fullscreen_display_mode);
            SDL_SetWindowFullscreen(_sdl_window_p, true);
            _fullscreen_mode = window_mode;
            _mode = window_mode;
        } break;
        case Mode::eBorderless: {
            SDL_SetWindowFullscreenMode(_sdl_window_p, nullptr);
            SDL_SetWindowFullscreen(_sdl_window_p, true);
            _fullscreen_mode = window_mode;
            _mode = window_mode;
        } break;
        case Mode::eWindowed: {
            SDL_SetWindowFullscreen(_sdl_window_p, false);
            _mode = window_mode;
        } break;
        default: break;
    }
}
void Window::set_mouse_relative(bool relative) {
    SDL_SetWindowRelativeMouseMode(_sdl_window_p, !relative);
    Input::register_mouse_relative(!relative);
}