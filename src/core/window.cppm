module;
#include <SDL3/SDL_video.h>
export module core.window;
import vulkan_hpp;
import core.input;

export struct Window {
    struct CreateInfo;
    enum Mode { eFullscreen, eBorderless, eWindowed };
    void init(const CreateInfo& info);
    void destroy();
    bool handle_event(void* event);

    void delay(uint32_t ms);
    void set_window_mode(Mode window_mode);
    void set_mouse_relative(bool relative);
    
    vk::Instance _instance;
    vk::SurfaceKHR _surface;
    SDL_Window* _sdl_window_p;
    // window state
    Mode _mode;
    Mode _fullscreen_mode;
    vk::Extent2D _size;
    SDL_DisplayMode _fullscreen_display_mode;
    bool _focused;
};
struct Window::CreateInfo {
    std::string name;
    vk::Extent2D size = { 1280, 720 };
    Window::Mode window_mode = Window::eWindowed;
    Window::Mode fullscreen_mode = Window::eBorderless;
};