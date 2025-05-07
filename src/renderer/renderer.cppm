export module renderer.renderer;
import core.device;
import renderer.swapchain;
import renderer.pipeline;
import renderer.semaphore;
import buffers.image;
import scene.scene;
import vulkan_hpp;
import smaa;

export struct Renderer {
    void init(Device& device, Scene& scene, vk::Extent2D extent, bool srgb_output);
    void destroy(Device& device);
    
    // resize internal buffers to match the new swapchain
    void resize(Device& device, Scene& scene, vk::Extent2D extent, bool srgb_output);
    // record command buffer and submit it to the universal queue. wait() needs to have been called before this
    void render(Device& device, Swapchain& swapchain, Scene& scene);
    // wait until device buffers are no longer in use and the command buffers can be recorded again
    void wait(Device& device);
    
private:
    void init_images(Device& device, vk::Extent2D extent);
    void init_pipelines(Device& device, vk::Extent2D extent, Scene& scene, bool srgb_output);
    void execute_pipes(vk::CommandBuffer cmd, Scene& scene);

private:
    // synchronization
    RendererSemaphore _synchronization;
    // command recording
    vk::CommandPool _command_pool;
    vk::CommandBuffer _command_buffer;
    // images
    DepthStencil _depth_stencil;
    Image _color;
    Image _storage;
    // pipelines
    Graphics _pipe_default;
    Compute _pipe_tone;
    SMAA _smaa;
    bool _smaa_enabled = true;
};