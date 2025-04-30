find_package(SDL3 3.2.10 CONFIG QUIET)
if (NOT SDL3_FOUND)
    set(SDL_STATIC OFF)
    set(SDL_SHARED ON)
    include(FetchContent)
    FetchContent_Declare(sdl
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
        GIT_TAG "release-3.2.10"
        GIT_SHALLOW ON
        OVERRIDE_FIND_PACKAGE
        EXCLUDE_FROM_ALL
        SYSTEM)
    FetchContent_MakeAvailable(sdl)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
