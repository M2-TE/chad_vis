module;
#include <cstdint>
#include <iostream>
export module engine;
import vulkan_hpp;
import window;
import device;
import swapchain;
import image;

export struct Engine {
    Engine() {
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
        
        // create swapchain
        _swapchain.init(_device, _window);

        // TODO: dynamically check this on resource creation
        // // set global properties relying on current device capabilities
        // DepthBuffer::set_format(_device._physical);
        // DepthStencil::set_format(_device._physical);
        // DeviceBuffer::set_staging_requirement(_vmalloc);
        // PipelineBase::set_module_deprecation(_device._physical);
    }
    ~Engine() {
        _swapchain.destroy(_device);
        _device.destroy();
        _window.destroy();
    }
    void run() {
        std::cout << "Hello from EngineCppm" << std::endl;
    }

    Window _window;
    Device _device;
    Swapchain _swapchain;
    //
    uint32_t _fps_foreground = 0;
    uint32_t _fps_background = 5;
    bool _rendering;
};