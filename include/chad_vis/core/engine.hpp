#pragma once
#include <print>
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>
#include "chad_vis/core/window.hpp"
#include "chad_vis/device/selector.hpp"
#include "chad_vis/device/image.hpp"

struct Engine {
    void init_instance() {

    }
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
        vk::PhysicalDeviceMaintenance5FeaturesKHR maintenance5 {
            .pNext = tail_p,
            .maintenance5 = vk::True,
        };
        if (device_selector.check_extension(_phys_device, vk::KHRMaintenance5ExtensionName)) tail_p = &maintenance5;
        vk::PhysicalDeviceMemoryPriorityFeaturesEXT memory_priority {
            .pNext = tail_p,
            .memoryPriority = vk::True,
        };
        if (device_selector.check_extension(_phys_device, vk::EXTMemoryPriorityExtensionName)) tail_p = &memory_priority;
        vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageable_memory {
            .pNext = tail_p,
            .pageableDeviceLocalMemory = vk::True,
        };
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

    }
    void destroy() {
        _queues.destroy(_device);
        _vmalloc.destroy();
        _device.destroy();
        _window.destroy();
    }
    void run() {
        while (true) {
            break;
        }
    }

    Window _window;
    dv::Queues _queues;
    vma::Allocator _vmalloc;
    vk::PhysicalDevice _phys_device;
    vk::Device _device;
};