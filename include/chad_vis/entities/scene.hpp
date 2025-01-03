#pragma once
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>
#include "chad_vis/device/queues.hpp"
#include "chad_vis/entities/camera.hpp"
// #include "chad_vis/entities/extra/grid.hpp"
#include "chad_vis/entities/extra/plymesh.hpp"

struct Scene {
    void init(vma::Allocator vmalloc, dv::Queues& queues) {
        _camera.init(vmalloc, queues._universal_i);

        // load mesh and grid objects
        _mesh.init(vmalloc, queues._universal_i, "data/v2/mesh.ply", glm::vec3{.1, .1, .1});
        // _grid.init(vmalloc, queues._universal_i, "data/v2/hashgrid.grid");
    }
    void destroy(vma::Allocator vmalloc) {
        _camera.destroy(vmalloc);

        // delete mesh and grid objects
        _mesh.destroy(vmalloc);
        // _grid.destroy(vmalloc);
    }

    // update without affecting current frames in flight
    void update_safe() {
        
    }
    // update after buffers are no longer being read
    void update(vma::Allocator vmalloc) {
        _camera.update(vmalloc);
    }

    Camera _camera;
    Plymesh _mesh;
    // Grid _grid;
};