#pragma once
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>
#include "chad_vis/entities/camera.hpp"
#include "chad_vis/device/queues.hpp"

struct Scene {
    void init(vma::Allocator vmalloc, dv::Queues& queues);
    void destroy(vma::Allocator vmalloc);

    // update without affecting current frames in flight
    void update_safe();
    // update after buffers are no longer being read
    void update(vma::Allocator vmalloc);

    Camera _camera;
};