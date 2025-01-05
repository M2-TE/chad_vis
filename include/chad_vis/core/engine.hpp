#pragma once
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>
#include "chad_vis/core/input.hpp"
#include "chad_vis/core/window.hpp"
#include "chad_vis/core/renderer.hpp"
#include "chad_vis/core/swapchain.hpp"
#include "chad_vis/device/image.hpp"
#include "chad_vis/device/queues.hpp"
#include "chad_vis/core/pipeline.hpp"
#include "chad_vis/device/selector.hpp"
#include "chad_vis/entities/scene.hpp"

struct Engine {
    void init() {
        // dynamic dispatcher init 1/3
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        // create a window, vulkan surface and instance
        _window.init(1280, 720, "CHAD Visualizer");

        // select physical device
        dv::Selector device_selector {
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
            ._required_queues {
                vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer,
                vk::QueueFlagBits::eGraphics,
                vk::QueueFlagBits::eCompute,
                vk::QueueFlagBits::eTransfer,
            }
        };
        _phys_device = device_selector.select_physical_device(_window._instance, _window._surface);

        // create logical device
        void* tail_p = nullptr;
        vk::PhysicalDeviceMaintenance5FeaturesKHR maintenance5 { .pNext = tail_p, .maintenance5 = vk::True };
        if (device_selector.check_extension(_phys_device, vk::KHRMaintenance5ExtensionName)) tail_p = &maintenance5;
        vk::PhysicalDeviceMemoryPriorityFeaturesEXT memory_priority { .pNext = tail_p, .memoryPriority = vk::True };
        if (device_selector.check_extension(_phys_device, vk::EXTMemoryPriorityExtensionName)) tail_p = &memory_priority;
        vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageable_memory {.pNext = tail_p, .pageableDeviceLocalMemory = vk::True };
        if (device_selector.check_extension(_phys_device, vk::EXTPageableDeviceLocalMemoryExtensionName)) tail_p = &pageable_memory;
        std::vector<uint32_t> queue_mappings;
        std::tie(_device, queue_mappings) = device_selector.create_logical_device(_phys_device, tail_p);

        // dynamic dispatcher init 3/3
        VULKAN_HPP_DEFAULT_DISPATCHER.init(_device);

        // create device queues
        _queues.init(_device, queue_mappings);

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
            .physicalDevice = _phys_device,
            .device = _device,
            .pVulkanFunctions = &vk_funcs,
            .instance = _window._instance,
            .vulkanApiVersion = vk::ApiVersion13,
        };
        _vmalloc = vma::createAllocator(info_vmalloc);

        // set the global properties for current physical device
        PipelineBase::set_module_deprecation(_phys_device);
        dv::DepthStencil::set_format(_phys_device);

        // set up swapchain, resize request will be fulfilled later
        _swapchain.set_target_framerate(_fps_foreground);
        _swapchain._resize_requested = true;
        _rendering = true;

        // create scene with renderable entities
        _scene.init(_vmalloc, _queues);
    }
    void destroy() {
        _device.waitIdle();
        //
        _scene.destroy(_vmalloc);
        _renderer.destroy(_device, _vmalloc);
        _vmalloc.destroy();
        //
        _queues.destroy(_device);
        _swapchain.destroy(_device);
        _device.destroy();
        //
        _window.destroy();
    }
    void run() {
        while (!glfwWindowShouldClose(_window._glfw_window_p)) {
            handle_events();
            if (!_rendering) _window.delay(50);
            if (!_rendering) continue;
            if (_swapchain._resize_requested) handle_resize();

            _scene.update_safe();
            _renderer.wait(_device);
            _scene.update(_vmalloc);
            _renderer.render(_device, _swapchain, _queues, _scene);
        }
    }

private:
    void handle_events() {
        // flush inputs from last frame before handling new events
        Input::flush();

        // check for changed window size and focus
        auto window_size = _window.size();
        auto window_focus = _window.focused();
        glfwPollEvents();
        if (window_size != _window.size()) handle_resize();
        if (window_focus != _window.focused()) {
            if (_window.focused()) {
                Input::Data::get().mouse_delta = {0, 0};
                _swapchain.set_target_framerate(_fps_foreground);
            }
            else  {
                Input::clear();
                _swapchain.set_target_framerate(_fps_background);
            }
        }

        // handle mouse grab
        if (Keys::pressed(GLFW_KEY_LEFT_ALT)) Mouse::set_mode(_window._glfw_window_p, false);
        else {
            if (Mouse::captured() && Keys::pressed(GLFW_KEY_ESCAPE)) Mouse::set_mode(_window._glfw_window_p, false);
            else if (!Mouse::captured() && Mouse::pressed(0)) Mouse::set_mode(_window._glfw_window_p, true);
            else if (!Mouse::captured() && Keys::released(GLFW_KEY_LEFT_ALT)) Mouse::set_mode(_window._glfw_window_p, true);
        }
    }
    void handle_resize() {
        _device.waitIdle();
        _scene._camera.resize(_window.size());
        _renderer.resize(_device, _vmalloc, _queues, _scene, _window.size());
        _swapchain.resize(_phys_device, _device, _window, _queues);
    }

    static Engine engine;
    Window _window;
    Swapchain _swapchain;
    Renderer _renderer;
    Scene _scene;
    //
    dv::Queues _queues;
    vma::Allocator _vmalloc;
    vk::PhysicalDevice _phys_device;
    vk::Device _device;
    //
    uint32_t _fps_foreground = 0;
    uint32_t _fps_background = 5;
    bool _rendering;
};