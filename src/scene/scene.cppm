module;
#include <glm/glm.hpp>
export module scene;
import vk_mem_alloc_hpp;
import plymesh;
import camera;
import grid;

export struct Scene {
    void init(vma::Allocator vmalloc) {
        _camera.init(vmalloc);

        // load mesh and grid objects
        _mesh.init(vmalloc, "data/mesh.ply", glm::vec3{.5, .5, .5});
        _grid.init(vmalloc, "data/hashgrid.grid");
    }
    void destroy(vma::Allocator vmalloc) {
        _camera.destroy(vmalloc);

        // delete mesh and grid objects
        _mesh.destroy(vmalloc);
        _grid.destroy(vmalloc);
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
    Grid _grid;
};