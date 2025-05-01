module;
#include <print>
#include <cstdint>
export module core.engine;
import core.swapchain;
import core.window;
import core.device;
import core.input;
import scene.scene;
import renderer.renderer;
import vulkan_hpp;

// temporary:
import buffers.device;
import renderer.pipeline;
import buffers.image;

export struct Engine {
    Engine();
    ~Engine();
    
    void handle_event(void* event_p);
    void handle_iteration();
    void handle_shortcuts();
    void handle_resize();

    Window _window;
    Device _device;
    Swapchain _swapchain;
    Renderer _renderer;
    Scene _scene;
    //
    uint32_t _fps_foreground = 0;
    uint32_t _fps_background = 5;
};

module: private;
Engine::Engine() {
    // create and open window
    _window.init({
        .name { "CHAD Visualizer" },
        .size { 1280, 720 },
        .window_mode = Window::eWindowed,
        .fullscreen_mode = Window::eBorderless,
    });
    
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
        ._required_features {},
        ._required_vk11_features {},
        ._required_vk12_features {
            .timelineSemaphore = true,
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
    PipelineBase::set_module_deprecation(_device._physical);

    _swapchain.init(_device, _window);
    _swapchain.set_target_framerate(_fps_foreground);
    _scene.init(_device._vmalloc);
    _scene._camera.resize(_window._size);
    _renderer.init(_device, _scene, _window._size, _swapchain._manual_srgb_required);
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

void Engine::handle_event(void* event_p) {
    bool resize_requested = _window.handle_event(event_p);
    if (resize_requested) _swapchain._resize_requested = true;
}
void Engine::handle_iteration() {
    handle_shortcuts();

    // handle window focus
    if (_window._focused) _swapchain.set_target_framerate(_fps_foreground);
    else _swapchain.set_target_framerate(_fps_background);
    if (_swapchain._resize_requested) {
        handle_resize();
        return;
    }
    
    _scene.update_safe();
    _renderer.wait(_device);
    _scene.update_unsafe(_device._vmalloc);
    _renderer.render(_device, _swapchain, _scene);
    Input::flush();
}
void Engine::handle_shortcuts() {
    // handle fullscreen controls
    if (Keys::pressed(Keys::eF11)) {
        switch (_window._mode) {
            case Window::eFullscreen: 
            case Window::eBorderless: _window.set_window_mode(Window::eWindowed); break;
            case Window::eWindowed: _window.set_window_mode(_window._fullscreen_mode); break;
        }
    }

    // handle mouse grab
    if (Keys::pressed(Keys::eLeftAlt)) {
        _window.set_mouse_relative(true);
    }
    else {
        if (Mouse::relative() && Keys::pressed(Keys::eEscape)) _window.set_mouse_relative(true);
        else if (!Mouse::relative() && !Keys::held(Keys::eLeftAlt) && Mouse::pressed(Mouse::eLeft)) _window.set_mouse_relative(false);
        else if (!Mouse::relative() && Keys::released(Keys::eLeftAlt)) _window.set_mouse_relative(false);
    }
}
void Engine::handle_resize() {
    // wait for current frame to finish
    _renderer.wait(_device);
    _device._logical.waitIdle();

    _scene._camera.resize(_window._size);
    _swapchain.resize(_device, _window);
    _renderer.resize(_device, _scene, _window._size, _swapchain._manual_srgb_required);
}