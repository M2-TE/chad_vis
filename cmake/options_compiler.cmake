if (MSVC)
    add_compile_options(
        "/diagnostics:classic"
        "/Zc:__cplusplus"
        "/Zc:inline"
        "/fp:except-"
        "/FC"
        "/Gm-"
        "/MP"
        "/W4"
        "/nologo")
    if (USE_STRICT_COMPILATION)
        add_compile_options("/WX")
    endif()
    if (USE_FAST_MATH)
        add_compile_options("/fp:fast")
    else()
        add_compile_options("/fp:precise")
    endif()
elseif (UNIX)
    # global compile options
    add_compile_options(
        "-march=native"
        "-Wall"
        "-Wextra"
        # "-Wpedantic"
        "-fno-strict-aliasing") # as recommended by Vulkan-Hpp
    if (USE_STRICT_COMPILATION)
        add_compile_options("-Werror")
    endif()
    if (USE_FAST_MATH)
        add_compile_options("-ffast-math")
    endif()

    # compiler specific flags
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options("-Wno-stringop-overflow")
        add_compile_options("-ffp-contract=off")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # none so far
    endif()
endif()