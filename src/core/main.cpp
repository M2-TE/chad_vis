import engine;
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>

SDL_AppResult SDL_AppInit(void **appstate_pp, int, char **) {
    *appstate_pp = new Engine();
    return SDL_AppResult::SDL_APP_CONTINUE;
}
SDL_AppResult SDL_AppIterate(void *appstate_p) {
    static_cast<Engine*>(appstate_p)->handle_iteration();
    return SDL_AppResult::SDL_APP_CONTINUE;
}
SDL_AppResult SDL_AppEvent(void *appstate_p, SDL_Event *event_p) {
    if (event_p->type == SDL_EventType::SDL_EVENT_QUIT) return SDL_AppResult::SDL_APP_SUCCESS;
    static_cast<Engine*>(appstate_p)->handle_event(event_p);
    return SDL_AppResult::SDL_APP_CONTINUE;
}
void SDL_AppQuit(void *appstate_p, SDL_AppResult) {
    delete static_cast<Engine*>(appstate_p);
}