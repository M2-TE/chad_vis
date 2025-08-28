module;
#include <glm/glm.hpp>
module scene.scene;

void Scene::init(vma::Allocator vmalloc) {
    _camera.init(vmalloc);

    // load mesh and grid objects
    _mesh.init(vmalloc, "v2/mesh.ply", glm::vec3{.5, .5, .5});
    // _grid.init(vmalloc, "v2/hashgrid.grid");
}
void Scene::destroy(vma::Allocator vmalloc) {
    _camera.destroy(vmalloc);

    // delete mesh and grid objects
    _mesh.destroy(vmalloc);
    // _grid.destroy(vmalloc);
}
void Scene::update_safe() {
    
}
void Scene::update_unsafe(vma::Allocator vmalloc) {
    _camera.update(vmalloc);
}