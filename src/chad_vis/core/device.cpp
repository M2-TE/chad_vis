#define VULKAN_HPP_USE_REFLECT
#include <vulkan/vulkan.hpp>
#include "chad_vis/core/device.hpp"

template<typename T>
bool check_features(const T& available, const T& required) {
    // track available features via simple vector
    std::vector<vk::Bool32> available_features;
    auto fnc_add_available = [&](auto arg) {
        if constexpr (std::is_same_v<decltype(arg), vk::Bool32>) {
            available_features.push_back(arg);
        }
    };
    std::apply([&](auto&... args) {
        ((fnc_add_available(args)), ...);
    }, available.reflect());

    // track required features via simple vector
    std::vector<vk::Bool32> required_features;
    auto fnc_add_required = [&](auto arg) {
        if constexpr (std::is_same_v<decltype(arg), vk::Bool32>) {
            required_features.push_back(arg);
        }
    };
    std::apply([&](auto&... args) {
        ((fnc_add_required(args)), ...);
    }, required.reflect());

    // check if all required features are available
    bool passed = true;
    for (size_t i = 0; i < available_features.size(); i++) {
        if (required_features[i] && !available_features[i]) passed = false;
    }
    return passed;
}
bool check_core_features(const Device::CreateInfo& info, vk::PhysicalDevice phys_device) {
    auto features = phys_device.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features>();
    bool passed = true;
    passed &= check_features(features.get<vk::PhysicalDeviceFeatures2>().features, info._required_features);
    passed &= check_features(features.get<vk::PhysicalDeviceVulkan11Features>(), info._required_vk11_features);
    passed &= check_features(features.get<vk::PhysicalDeviceVulkan12Features>(), info._required_vk12_features);
    passed &= check_features(features.get<vk::PhysicalDeviceVulkan13Features>(), info._required_vk13_features);
    return passed;
}
bool check_extensions(std::set<std::string> required_extensions, vk::PhysicalDevice physical_device) {
    auto ext_props = physical_device.enumerateDeviceExtensionProperties();
    for (auto& extension: ext_props) {
        std::string ext_name = extension.extensionName;
        auto ext_it = required_extensions.find(ext_name);
        // erase from set if found
        if (ext_it != required_extensions.end()) {
            required_extensions.erase(ext_it);
        }
    }
    // print missing extensions, if any
    for (auto& extension: required_extensions) {
        std::println("\tMissing device extension: {}", extension);
    }
    // pass if all required extensions were erased
    bool passed = required_extensions.size() == 0;
    return passed;
}
bool check_api_ver(const Device::CreateInfo& info, vk::PhysicalDeviceProperties& props) {
    bool passed = true;
    if (vk::apiVersionMajor(props.apiVersion) < info._required_major) passed = false;
    if (vk::apiVersionMinor(props.apiVersion) < info._required_minor) passed = false;
    if (!passed) std::println("\tMissing vulkan {}.{} support", info._required_major, info._required_minor);
    return passed;
}
auto get_queue_families(vk::PhysicalDevice physical_device) -> std::vector<uint32_t> {
    // require 4 types of queues: universal, graphics, compute, transfer
    std::vector<vk::QueueFlags> required_queues = {
        vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer,
        vk::QueueFlagBits::eGraphics,
        vk::QueueFlagBits::eCompute,
        vk::QueueFlagBits::eTransfer,
    };
    // <queue family index, count of queue capabilities>
    typedef std::pair<uint32_t, uint32_t> QueueCount;
    std::vector<std::vector<QueueCount>> queue_family_counters;
    queue_family_counters.resize(required_queues.size());
    // iterate through all queue families to store relevant ones
    auto queue_family_props = physical_device.getQueueFamilyProperties();
    for (uint32_t i = 0; i < queue_family_props.size(); i++) {
        vk::QueueFlags flags = queue_family_props[i].queueFlags;
        uint32_t capability_count = std::popcount((uint32_t)flags);
        // check if this queue family works for each requested queue
        for (size_t q = 0; q < required_queues.size(); q++) {
            vk::QueueFlags masked = flags & required_queues[q];
            if (masked == required_queues[q]) {
                queue_family_counters[q].emplace_back(i, capability_count);
            }
        }
    }
    // track unique queue families
    std::set<uint32_t> unique_queue_families;
    // store final queue family indices for requested queues
    std::vector<uint32_t> queue_families;
    for (size_t i = 0; i < required_queues.size(); i++) {
        // sort queues by capability count
        auto& vec = queue_family_counters[i];
        std::sort(vec.begin(), vec.end(), [](QueueCount& a, QueueCount& b){
            return a.second < b.second;
        });
        // use the queue with the fewest capabilities possible
        uint32_t queue_family_index = vec.front().first;
        // create new queue family info if unique
        auto [unique_it, _] = unique_queue_families.emplace(queue_family_index);
        // add to final queue family list
        queue_families.push_back(*unique_it);
    }
    return queue_families;
}
bool check_presentation(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
    // check if presentation capabilties are required
    if (surface == nullptr) return true;

    // create dummy queues to check presentation capabilities
    auto queue_families = get_queue_families(physical_device);
    vk::Bool32 passed = false;
    // check each queue family for presentation capabilities
    for (auto& queue_family: queue_families) {
        vk::Bool32 b = physical_device.getSurfaceSupportKHR(queue_family, surface);
        passed |= b;
    }

    if (!passed) std::print("Missing presentation capabilities");
    return passed;
}
bool check_extension(vk::PhysicalDevice physical_device, const char* extension_name) {
    auto available_extensions = physical_device.enumerateDeviceExtensionProperties();
    for (auto& available: available_extensions) {
        if (strcmp(extension_name, available.extensionName) == 0) {
            return true;
        }
    }
    return false;
}

auto create_physical(const Device::CreateInfo& info) -> vk::PhysicalDevice {
    // enumerate devices
    auto phys_devices = info._instance.enumeratePhysicalDevices();
    if (phys_devices.size() == 0) std::println("No device with vulkan support found");

    // create a std::set from the required extensions
    std::set<std::string> required_extensions {
        info._required_extensions.cbegin(),
        info._required_extensions.cend()
    };
    
    // check for matching devices (bool = is_preferred, vk::DeviceSize = memory_size)
    std::vector<std::pair<vk::PhysicalDevice, vk::DeviceSize>> matching_devices;
    std::println("Available devices:");
    for (vk::PhysicalDevice device: phys_devices) {
        auto props = device.getProperties();
        bool passed = true;
        passed &= check_extensions(required_extensions, device);
        passed &= check_api_ver(info, props);
        passed &= check_core_features(info, device);
        passed &= check_presentation(device, info._surface);

        // add device candidate if it passed tests
        if (passed) {
            vk::DeviceSize memory_size = 0;
            auto memory_props = device.getMemoryProperties();
            for (auto& heap: memory_props.memoryHeaps) {
                if (heap.flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
                    memory_size += heap.size;
                }
            }
            matching_devices.emplace_back(device, memory_size);
        }
        std::string pass_str = passed ? "passed" : "failed";
        std::println("-> {}: {}", pass_str, (const char*)props.deviceName);
    }

    // optionally bail out
    if (matching_devices.size() == 0) {
        std::println("None of the devices match the requirements");
        exit(0);
    }

    // sort devices by favouring certain gpu types and large local memory heaps
    typedef std::pair<vk::PhysicalDevice, vk::DeviceSize> DeviceEntry;
    auto fnc_sorter = [&](DeviceEntry& a, DeviceEntry& b) {
        return a.second > b.second;
    };
    std::sort(matching_devices.begin(), matching_devices.end(), fnc_sorter);
    vk::PhysicalDevice phys_device = std::get<0>(matching_devices.front());
    std::println("Picked device: {}", (const char*)phys_device.getProperties().deviceName);
    return phys_device;
}
auto create_logical(const Device::CreateInfo& info, vk::PhysicalDevice physical_device, std::vector<uint32_t>& queue_families) -> vk::Device {
    // set up features
    vk::PhysicalDeviceFeatures2 required_features {
        .features = info._required_features,
    };
    vk::PhysicalDeviceVulkan11Features features_vk11 = info._required_vk11_features;
    vk::PhysicalDeviceVulkan12Features features_vk12 = info._required_vk12_features;
    vk::PhysicalDeviceVulkan13Features features_vk13 = info._required_vk13_features;

    // chain main features
    void** tail_pp = &required_features.pNext;
    if (info._required_minor >= 1) {
        *tail_pp = &features_vk11;
        tail_pp = &features_vk11.pNext;
    }
    if (info._required_minor >= 2) {
        *tail_pp = &features_vk12;
        tail_pp = &features_vk12.pNext;
    }
    if (info._required_minor >= 3) {
        *tail_pp = &features_vk13;
        tail_pp = &features_vk13.pNext;
    }

    // enable optional extensions if available
    auto extensions = info._required_extensions;
    auto available_extensions = physical_device.enumerateDeviceExtensionProperties();
    for (const char* ext: info._optional_extensions) {
        for (auto& available: available_extensions) {
            if (strcmp(ext, available.extensionName) == 0) {
                extensions.push_back(ext);
                break;
            }
        }
    }

    // chain additional features if their extensions are available
    for (auto& [feature_void_p, ext]: info._optional_features) {
        if (check_extension(physical_device, ext)) {
            // simply reinterpret to get the pNext member
            vk::PhysicalDeviceFeatures2* feature_p = reinterpret_cast<vk::PhysicalDeviceFeatures2*>(feature_void_p);
            *tail_pp = feature_p;
            tail_pp = &feature_p->pNext;
        }
    }
    
    // tally unique queues
    std::map<uint32_t, uint32_t> queue_counts;
    for (auto& queue_family: queue_families) {
        queue_counts[queue_family]++;
    }
    // create queue family infos
    std::vector<vk::DeviceQueueCreateInfo> info_queues;
    float queue_priority = 1.0f;
    for (auto& [queue_family, count]: queue_counts) {
        info_queues.push_back({
            .queueFamilyIndex = queue_family,
            .queueCount = 1, // just hardcore to single queue of each type for now
            .pQueuePriorities = &queue_priority,
        });
    }

    // create device
    vk::DeviceCreateInfo info_device {
        .pNext = &required_features,
        .queueCreateInfoCount = (uint32_t)info_queues.size(),
        .pQueueCreateInfos = info_queues.data(),
        .enabledExtensionCount = (uint32_t)extensions.size(),
        .ppEnabledExtensionNames = extensions.data(),
    };
    return physical_device.createDevice(info_device);
}

void Device::init(const Device::CreateInfo& info) {
    // select physical device
    _physical = create_physical(info);

    // create logical device from physical
    auto queue_families = get_queue_families(_physical);
    _logical = create_logical(info, _physical, queue_families);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(_logical);

    // assign queue families
    _universal_i = queue_families[0];
    _graphics_i = queue_families[1];
    _compute_i = queue_families[2];
    _transfer_i = queue_families[3];

    // get queues
    _universal_queue = _logical.getQueue(_universal_i, 0);
    _graphics_queue = _logical.getQueue(_graphics_i, 0);
    _compute_queue = _logical.getQueue(_compute_i, 0);
    _transfer_queue = _logical.getQueue(_transfer_i, 0);

    // create command pools
    _universal_pool = _logical.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = _universal_i,
    });
    _graphics_pool = _logical.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = _graphics_i,
    });
    _compute_pool = _logical.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = _compute_i,
    });
    _transfer_pool = _logical.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = _transfer_i,
    });
}
void Device::destroy() {
    _logical.destroyCommandPool(_universal_pool);
    _logical.destroyCommandPool(_graphics_pool);
    _logical.destroyCommandPool(_compute_pool);
    _logical.destroyCommandPool(_transfer_pool);
    _logical.destroy();
}

auto Device::oneshot_begin() -> vk::CommandBuffer {
    vk::CommandBufferAllocateInfo info = {
        .commandPool = _universal_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };
    vk::CommandBuffer cmd = _logical.allocateCommandBuffers(info)[0];
    cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    return cmd;
}
void Device::oneshot_end(vk::CommandBuffer cmd, const vk::ArrayProxy<vk::Semaphore>& sign_semaphores) {
    cmd.end();
    vk::SubmitInfo info = {
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = (uint32_t)sign_semaphores.size(),
        .pSignalSemaphores = sign_semaphores.data(),
    };
    _universal_queue.submit(info);
    _universal_queue.waitIdle();
    _logical.freeCommandBuffers(_universal_pool, cmd);
}
