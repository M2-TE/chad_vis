# query SIMD support
# FetchContent_Declare(libsimdpp
#     GIT_REPOSITORY "https://github.com/p12tic/libsimdpp.git"
#     GIT_TAG "v2.1"
#     GIT_SHALLOW ON
#     SOURCE_SUBDIR "disabled")
# FetchContent_MakeAvailable(libsimdpp)
# list(APPEND CMAKE_MODULE_PATH "${libsimdpp_SOURCE_DIR}/cmake/")
# include(SimdppMultiarch)
# simdpp_get_runnable_archs(ARCHS)

# use SIMD support info to configure glm
# set(GLM_ENABLE_SIMD_SSE2   ${CAN_RUN_X86_SSE2})
# set(GLM_ENABLE_SIMD_SSE3   ${CAN_RUN_X86_SSE3})
# set(GLM_ENABLE_SIMD_SSSE3  ${CAN_RUN_X86_SSSE3})
# set(GLM_ENABLE_SIMD_SSE4_1 ${CAN_RUN_X86_SSE4_1})
# set(GLM_ENABLE_SIMD_SSE4_2 ${CAN_RUN_X86_SSE4_2})
# set(GLM_ENABLE_SIMD_AVX    ${CAN_RUN_X86_AVX})
# set(GLM_ENABLE_SIMD_AVX2   ${CAN_RUN_X86_AVX2})

# standard glm options
if(${CMAKE_CXX_STANDARD} GREATER_EQUAL 23)
    set(GLM_ENABLE_CXX_23 ON)
elseif(${CMAKE_CXX_STANDARD} EQUAL 20)
    set(GLM_ENABLE_CXX_20 ON)
elseif(${CMAKE_CXX_STANDARD} EQUAL 17)
    set(GLM_ENABLE_CXX_17 ON)
elseif(${CMAKE_CXX_STANDARD} EQUAL 14)
    set(GLM_ENABLE_CXX_14 ON)
elseif(${CMAKE_CXX_STANDARD} EQUAL 11)
    set(GLM_ENABLE_CXX_11 ON)
elseif(${CMAKE_CXX_STANDARD} EQUAL 98)
    set(GLM_ENABLE_CXX_98 ON)
endif()
set(GLM_BUILD_LIBRARY ON)
set(GLM_BUILD_TEST OFF)
set(GLM_BUILD_INSTALL OFF)
if(${CROSS_PLATFORM_DETERMINISTIC})
    set(GLM_ENABLE_FAST_MATH OFF)
else()
    set(GLM_ENABLE_FAST_MATH ON)
endif()
set(GLM_ENABLE_LANG_EXTENSIONS ${CMAKE_CXX_EXTENSIONS})
set(GLM_DISABLE_AUTO_DETECTION OFF)
set(GLM_FORCE_PURE OFF)

FetchContent_Declare(glm
    GIT_REPOSITORY "https://github.com/g-truc/glm.git"
    GIT_TAG "18feaec455f8ee1f256d91f515fb02d66268ba14" # basically 1.0.2 tag
    # GIT_TAG "1.0.1"
    # GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(glm)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "GLM_FORCE_DEPTH_ZERO_TO_ONE"
    "GLM_FORCE_ALIGNED_GENTYPES"
    "GLM_FORCE_INTRINSICS")
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)