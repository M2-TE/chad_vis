#pragma once
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>
#include "chad_vis/core/input.hpp"
#include "chad_vis/core/device.hpp"
#include "chad_vis/core/window.hpp"
#include "chad_vis/core/renderer.hpp"
#include "chad_vis/core/pipeline.hpp"
#include "chad_vis/core/swapchain.hpp"
#include "chad_vis/device/image.hpp"
#include "chad_vis/entities/scene.hpp"

struct Engine {
    void init() {
        // dynamic dispatcher init 1/3
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        // create a window, vulkan surface and instance
        _window.init(1280, 720, "CHAD Visualizer");
        glfwSetWindowUserPointer(_window._glfw_window_p, this);

        // select physical device, then create logical device and its queues
        vk::PhysicalDeviceMaintenance5FeaturesKHR maintenance5 { .maintenance5 = vk::True };
        vk::PhysicalDeviceMemoryPriorityFeaturesEXT memory_priority { .memoryPriority = vk::True };
        vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageable_memory { .pageableDeviceLocalMemory = vk::True };
        _device.init({
            ._instance = _window._instance,
            ._surface = _window._surface,
            ._required_major = 1,
            ._required_minor = 3,
            ._preferred_device_type = vk::PhysicalDeviceType::eIntegratedGpu,
            ._required_extensions {
                vk::KHRSwapchainExtensionName,
            },
            ._optional_extensions {
                vk::KHRMaintenance5ExtensionName,
                vk::EXTMemoryBudgetExtensionName,
                vk::EXTMemoryPriorityExtensionName,
                vk::EXTPageableDeviceLocalMemoryExtensionName,
            },
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
            ._optional_features {
                {&maintenance5, vk::KHRMaintenance5ExtensionName},
                {&memory_priority, vk::EXTMemoryPriorityExtensionName},
                {&pageable_memory, vk::EXTPageableDeviceLocalMemoryExtensionName},
            }
        });

        // create vulkan memory allocator
        vma::VulkanFunctions vk_funcs {
            .vkGetInstanceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr,
        };
        vma::AllocatorCreateInfo info_vmalloc {
            .flags =
                vma::AllocatorCreateFlagBits::eKhrBindMemory2 |
                vma::AllocatorCreateFlagBits::eKhrMaintenance4 |
                vma::AllocatorCreateFlagBits::eKhrMaintenance5 |
                vma::AllocatorCreateFlagBits::eExtMemoryBudget |
                vma::AllocatorCreateFlagBits::eExtMemoryPriority |
                vma::AllocatorCreateFlagBits::eBufferDeviceAddress |
                vma::AllocatorCreateFlagBits::eKhrDedicatedAllocation,
            .physicalDevice = _device._physical,
            .device = _device._logical,
            .pVulkanFunctions = &vk_funcs,
            .instance = _window._instance,
            .vulkanApiVersion = vk::ApiVersion13,
        };
        _vmalloc = vma::createAllocator(info_vmalloc);

        // set the global properties for current physical device
        PipelineBase::set_module_deprecation(_device._physical);
        DepthStencil::set_format(_device._physical);
        DepthBuffer::set_format(_device._physical);
        // figure out whether ReBAR is available based on single large allocation
        DeviceBuffer::set_staging_requirement(_vmalloc);

        // create scene with renderable entities
        _scene.init(_vmalloc);

        // first resize will set up swapchain and renderer
        _swapchain.set_target_framerate(_fps_foreground);
        handle_resize();
    }
    void destroy() {
        _device._logical.waitIdle();
        //
        _scene.destroy(_vmalloc);
        _renderer.destroy(_device, _vmalloc);
        _vmalloc.destroy();
        //
        _swapchain.destroy(_device);
        _device.destroy();
        //
        _window.destroy();
    }
    void run() {
        _rendering = true;
        while (!glfwWindowShouldClose(_window._glfw_window_p)) {
            handle_events();
            if (!_rendering) _window.delay(50);
            if (!_rendering) continue;
            if (_swapchain._resize_requested) handle_resize();

            _scene.update_safe();
            _renderer.wait(_device);
            _scene.update(_vmalloc);
            _renderer.render(_device, _swapchain, _scene);
        }
    }
    
private:
    void handle_events() {
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
    void handle_resize() {
        _device._logical.waitIdle();
        _scene._camera.resize(_window.size());
        _swapchain.resize(_device, _window);
        _renderer.resize(_device, _vmalloc, _scene, _window.size(), _swapchain._srgb_required);
    }

    vma::Allocator _vmalloc;
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