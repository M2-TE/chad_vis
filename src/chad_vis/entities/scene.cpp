#include "chad_vis/entities/scene.hpp"

void Scene::init(vma::Allocator vmalloc, dv::Queues& queues) {
    _camera.init(vmalloc, queues._universal_i);
}
void Scene::destroy(vma::Allocator vmalloc) {
    _camera.destroy(vmalloc);
}

// update without affecting current frames in flight
void Scene::update_safe() {
    
}
// update after buffers are no longer being read
void Scene::update(vma::Allocator vmalloc) {
    _camera.update(vmalloc);
}