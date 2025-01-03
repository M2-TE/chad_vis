#pragma once
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>
#include "chad_vis/core/input.hpp"
#include "chad_vis/core/window.hpp"
#include "chad_vis/core/renderer.hpp"
#include "chad_vis/core/swapchain.hpp"
#include "chad_vis/device/image.hpp"
#include "chad_vis/device/queues.hpp"
#include "chad_vis/device/pipeline.hpp"
#include "chad_vis/device/selector.hpp"
#include "chad_vis/entities/scene.hpp"

struct Engine {
    void init() {
        // dynamic dispatcher init 1/3
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        // create a window, vulkan surface and instance
        _window.init(1280, 720, "CHAD Visualizer");

        // dynamic dispatcher init 2/3
        VULKAN_HPP_DEFAULT_DISPATCHER.init(_window._instance);

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

        // set the global format for depth stencil images
        dv::DepthStencil::set_format(_phys_device);
        dv::PipelineBase::set_module_deprecation(_phys_device);

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
        while (_window._sfml_window.isOpen()) {
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
        // flush inputs from last frame
        Input::flush();

        // handle all events from the SFML window
        while (const std::optional event = _window._sfml_window.pollEvent()) {
            Input::handle_event(event);
            if (event->is<sf::Event::Closed>()) {
                _device.waitIdle();
                _window._sfml_window.close();
                _rendering = false;
            }
            else if (event->is<sf::Event::FocusLost>()) _swapchain.set_target_framerate(_fps_background);
            else if (event->is<sf::Event::FocusGained>()) {
                _swapchain.set_target_framerate(_fps_foreground);
                _window._sfml_window.setMouseCursorGrabbed(Input::Mouse::captured());
                _window._sfml_window.setMouseCursorVisible(!Input::Mouse::captured());
            }
        }

        // handle mouse grab
        if (Input::Keys::pressed(sf::Keyboard::Scan::LAlt)) {
            _window._sfml_window.setMouseCursorGrabbed(false);
            _window._sfml_window.setMouseCursorVisible(true);
            Input::register_capture(false);
        }
        else {
            // while alt is not held, allow grab via click and ungrab via escape
            if (Input::Mouse::captured() && Input::Keys::pressed(sf::Keyboard::Scan::Escape)) {
                _window._sfml_window.setMouseCursorGrabbed(false);
                _window._sfml_window.setMouseCursorVisible(true);
                Input::register_capture(false);
            }
            else if (!Input::Mouse::captured() && Input::Mouse::pressed(sf::Mouse::Button::Left)) {
                _window._sfml_window.setMouseCursorGrabbed(true);
                _window._sfml_window.setMouseCursorVisible(false);
                Input::register_capture(true);
            }
            else if (!Input::Mouse::captured() && Input::Keys::released(sf::Keyboard::Scan::LAlt)) {
                _window._sfml_window.setMouseCursorGrabbed(true);
                _window._sfml_window.setMouseCursorVisible(false);
                Input::register_capture(true);
            }
        }
    }
    void handle_resize() {
        _device.waitIdle();
        _scene._camera.resize(_window.size());
        _renderer.resize(_device, _vmalloc, _queues, _scene, _window.size());
        _swapchain.resize(_phys_device, _device, _window, _queues);
    }

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