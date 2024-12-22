FetchContent_Declare(sfml
    GIT_REPOSITORY "https://github.com/SFML/SFML.git"
    GIT_TAG "3.0.0"
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(sfml)
target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Window)