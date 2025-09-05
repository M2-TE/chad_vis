export module core.engine;
import vulkan_hpp;
import core.window;
import core.device;
import renderer.swapchain;
import renderer.renderer;
import scene.scene;

export struct Engine {
    Engine();
    ~Engine();
    
    void handle_event(void* event_p);
    void handle_iteration();
    void handle_shortcuts();
    void handle_resize();

    Window _window;
    Device _device;
    Swapchain _swapchain;
    Renderer _renderer;
    Scene _scene;
    //
    uint32_t _fps_foreground = 0;
    uint32_t _fps_background = 5;
};