module;
#include <cstdint>
#include <GLFW/glfw3.h>
export module engine;
import vulkan_hpp;
import swapchain;
import renderer;
import window;
import device;
import input;
import scene;

// temporary:
import device_buffer;
import image;
import pipeline;


export struct Engine {
    Engine();
    ~Engine();
    void run();

private:
    void handle_events();
    void handle_resize();

    Window _window;
    Device _device;
    Swapchain _swapchain;
    Renderer _renderer;
    Scene _scene;
    //
    uint32_t _fps_foreground = 0;
    uint32_t _fps_background = 5;
    bool _rendering;
};

module: private;
Engine::Engine() {
    // create and open window
    _window.init(1280, 720, "CHAD Visualizer");
    
    // select physical and create logical device
    vk::PhysicalDeviceMaintenance5FeaturesKHR maintenance5 { .maintenance5 = vk::True };
    vk::PhysicalDeviceMemoryPriorityFeaturesEXT memory_priority { .memoryPriority = vk::True };
    vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageable_memory { .pageableDeviceLocalMemory = vk::True };
    _device.init({
        ._instance = _window._instance,
        ._surface = _window._surface,
        ._required_major = 1,
        ._required_minor = 3,
        ._preferred_device_type = vk::PhysicalDeviceType::eIntegratedGpu,
        ._required_features {
        },
        ._required_vk11_features {
        },
        ._required_vk12_features {
            .bufferDeviceAddress = true,
        },
        ._required_vk13_features {
            .synchronization2 = true,
            .dynamicRendering = true,
            .maintenance4 = true,
        },
        ._required_extensions {
            vk::KHRSwapchainExtensionName,
        },
        ._optional_extensions {
            vk::EXTMemoryBudgetExtensionName,
        },
        ._optional_features {
            {&maintenance5, vk::KHRMaintenance5ExtensionName},
            {&memory_priority, vk::EXTMemoryPriorityExtensionName},
            {&pageable_memory, vk::EXTPageableDeviceLocalMemoryExtensionName},
        }
    });
    
    // set global properties relying on current device capabilities
    DepthBuffer::set_format(_device._physical);
    DepthStencil::set_format(_device._physical);
    DeviceBuffer::set_staging_requirement(_device._vmalloc);
    Graphics::set_module_deprecation(_device._physical);

    _swapchain.init(_device, _window);
    _swapchain.set_target_framerate(_fps_foreground);
    _scene.init(_device._vmalloc);
    _scene._camera.resize(_window.size());
    _renderer.init(_device, _scene, _window.size(), _swapchain._manual_srgb_required);
}
Engine::~Engine() {
    // wait for current frame to finish
    _renderer.wait(_device);
    _device._logical.waitIdle();

    // shut down components
    _scene.destroy(_device._vmalloc);
    _renderer.destroy(_device);
    _swapchain.destroy(_device);
    _device.destroy();
    _window.destroy();
}
void Engine::run() {
    _rendering = true;
    while (!glfwWindowShouldClose(_window._glfw_window_p)) {
        handle_events();
        if (!_rendering) _window.delay(50);
        if (!_rendering) continue;
        if (_swapchain._resize_requested) handle_resize();

        _scene.update_safe();
        _renderer.wait(_device);
        _scene.update(_device._vmalloc);
        _renderer.render(_device, _swapchain, _scene);
    }
}
void Engine::handle_events() {
    // flush inputs from last frame before handling new events
    Input::flush();

    // check for changed window size and focus
    auto window_size = _window.size();
    auto window_focused = _window.focused();
    glfwPollEvents();

    // handle window resize
    if (window_size != _window.size()) handle_resize();
    // handle window focus
    if (window_focused != _window.focused()) {
        if (_window.focused()) {
            Input::Data::get().mouse_delta = {0, 0};
            _swapchain.set_target_framerate(_fps_foreground);
        }
        else {
            Input::clear();
            _swapchain.set_target_framerate(_fps_background);
        }
    }
    // handle minimize (iconify)
    if (_window.minimized()) _rendering = false;
    else _rendering = true;

    // handle fullscreen controls
    if (Keys::pressed(GLFW_KEY_F11)) _window.toggle_window_mode();

    // handle mouse grab
    if (Keys::pressed(GLFW_KEY_LEFT_ALT)) Mouse::set_mode(_window._glfw_window_p, false);
    else {
        if (Mouse::captured() && Keys::pressed(GLFW_KEY_ESCAPE)) Mouse::set_mode(_window._glfw_window_p, false);
        else if (!Mouse::captured() && !Keys::held(GLFW_KEY_LEFT_ALT) && Mouse::pressed(0)) Mouse::set_mode(_window._glfw_window_p, true);
        else if (!Mouse::captured() && Keys::released(GLFW_KEY_LEFT_ALT)) Mouse::set_mode(_window._glfw_window_p, true);
    }
}
void Engine::handle_resize() {
    _device._logical.waitIdle();
    _scene._camera.resize(_window.size());
    _swapchain.resize(_device, _window);
    _renderer.resize(_device, _scene, _window.size(), _swapchain._manual_srgb_required);
}