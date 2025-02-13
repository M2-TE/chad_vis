module;
#include <cstdint>
export module render_semaphore;
import vulkan_hpp;
import device;

export struct RenderSemaphore {
    void init(Device& device) {
        vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo> chain_timeline {
            {}, { .semaphoreType = vk::SemaphoreType::eTimeline, .initialValue = 0 }
        };
        _value = device._logical.createSemaphore(chain_timeline.get());
        _ready_to_write = 0;
        _ready_to_read = _ready_to_write + 1;
    }
    void destroy(Device& device) {
        device._logical.destroySemaphore(_value);
    }
    void wait_ready_to_read(Device& device) {
        vk::SemaphoreWaitInfo info_wait {
            .semaphoreCount = 1,
            .pSemaphores = &_value,
            .pValues = &_ready_to_read,
        };
        while (vk::Result::eTimeout == device._logical.waitSemaphores({ info_wait }, UINT64_MAX)) {};
    }
    void wait_ready_to_write(Device& device) {
        vk::SemaphoreWaitInfo info_wait {
            .semaphoreCount = 1,
            .pSemaphores = &_value,
            .pValues = &_ready_to_write,
        };
        while (vk::Result::eTimeout == device._logical.waitSemaphores({ info_wait }, UINT64_MAX)) {};
    }
    vk::Semaphore _value;
    uint64_t _ready_to_write;
    uint64_t _ready_to_read;
};