export module renderer.semaphore;
import vulkan_hpp;
import core.device;

export struct RendererSemaphore {
    void init(Device& device) {
        vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo> chain_timeline {
            {}, { .semaphoreType = vk::SemaphoreType::eTimeline, .initialValue = 0 }
        };
        _semaphore = device._logical.createSemaphore(chain_timeline.get());
        _val_ready_to_write = 0;
        _val_ready_to_read = 0;
    }
    void destroy(Device& device) {
        device._logical.destroySemaphore(_semaphore);
    }
    // increase wait value for future write
    void prepare_for_read() {
        _val_ready_to_write = _val_ready_to_read + 1;
    }
    // increase wait value for future read
    void prepare_for_write() {
        _val_ready_to_read = _val_ready_to_write + 1;
    }
    // wait on the host thread until reading is permitted
    void wait_ready_to_read(Device& device) {
        vk::SemaphoreWaitInfo info_wait {
            .semaphoreCount = 1,
            .pSemaphores = &_semaphore,
            .pValues = &_val_ready_to_read,
        };
        while (vk::Result::eTimeout == device._logical.waitSemaphores({ info_wait }, UINT64_MAX)) {};
    }
    // wait on the host thread until writing is permitted
    void wait_ready_to_write(Device& device) {
        vk::SemaphoreWaitInfo info_wait {
            .semaphoreCount = 1,
            .pSemaphores = &_semaphore,
            .pValues = &_val_ready_to_write,
        };
        while (vk::Result::eTimeout == device._logical.waitSemaphores({ info_wait }, UINT64_MAX)) {};
    }
    vk::Semaphore _semaphore;
    uint64_t _val_ready_to_write;
    uint64_t _val_ready_to_read;
};