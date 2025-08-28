export module scene.scene;
import scene.grid;
import scene.camera;
import scene.plymesh;
import vulkan_ma_hpp;

export struct Scene {
    void init(vma::Allocator vmalloc);
    void destroy(vma::Allocator vmalloc);

    // update without affecting current frames in flight
    void update_safe();
    // update after buffers are no longer being read
    void update_unsafe(vma::Allocator vmalloc);

    Camera _camera;
    Plymesh _mesh;
    // Grid _grid;
};