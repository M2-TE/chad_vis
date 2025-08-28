include(FetchContent)
FetchContent_Declare(cme
    GIT_REPOSITORY "https://github.com/M2-TE/cme.git"
    GIT_TAG "v1.2.1"
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(cme)

cme_create_library(datasets STATIC CXX_MODULE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/datasets")
target_link_libraries(${PROJECT_NAME} PRIVATE cme::datasets)