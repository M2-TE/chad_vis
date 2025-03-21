module;
#include <glm/glm.hpp>
export module scene;
import vma_hpp;
import plymesh;
import camera;
import grid;

export struct Scene {
    void init(vma::Allocator vmalloc);
    void destroy(vma::Allocator vmalloc);

    // update without affecting current frames in flight
    void update_safe();
    // update after buffers are no longer being read
    void update_unsafe(vma::Allocator vmalloc);

    Camera _camera;
    Plymesh _mesh;
    Grid _grid;
};

module: private;
void Scene::init(vma::Allocator vmalloc) {
    _camera.init(vmalloc);

    // load mesh and grid objects
    _mesh.init(vmalloc, "data/v2/mesh.ply", glm::vec3{.5, .5, .5});
    _grid.init(vmalloc, "data/v2/hashgrid.grid");
}
void Scene::destroy(vma::Allocator vmalloc) {
    _camera.destroy(vmalloc);

    // delete mesh and grid objects
    _mesh.destroy(vmalloc);
    _grid.destroy(vmalloc);
}
void Scene::update_safe() {
    
}
void Scene::update_unsafe(vma::Allocator vmalloc) {
    _camera.update(vmalloc);
}