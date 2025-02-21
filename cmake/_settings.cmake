# policies
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) # disallow option() from overwriting set()

# cxx settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# cmake settings
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF)

# IPO/LTO support
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
include(CheckIPOSupported)
check_ipo_supported(RESULT CMAKE_INTERPROCEDURAL_OPTIMIZATION LANGUAGES CXX)
message(STATUS "IPO/LTO enabled: ${CMAKE_INTERPROCEDURAL_OPTIMIZATION}")

# other
set(STRICT_COMPILATION OFF) # all warnings will emit errors
set(CROSS_PLATFORM_DETERMINISTIC OFF) # cross-platform determinism disables fast-math

if(MSVC)
    set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE "x64")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # static msvc runtime lib
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus /Gm- /MP /nologo /diagnostics:classic /FC /fp:except-")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:inline")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    if (STRICT_COMPILATION)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
    endif()
    if (CROSS_PLATFORM_DETERMINISTIC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
    endif()
    
	# Set compiler flags for various configurations
	set(CMAKE_CXX_FLAGS_DEBUG "/GS /Od /Ob0 /RTC1")
	set(CMAKE_CXX_FLAGS_RELEASE "/GS- /Gy /O2 /Oi /Ot")
    # set(CMAKE_EXE_LINKER_FLAGS "/SUBSYSTEM:WINDOWS")
    set(CMAKE_EXE_LINKER_FLAGS "/SUBSYSTEM:CONSOLE") # to get console output on windows
    set(CMAKE_EXE_LINKER_FLAGS "/ignore:4221")
elseif(UNIX)
    # enable mold linker if present
    find_program(MOLD_FOUND mold)
    if (MOLD_FOUND)
        message(STATUS "Using mold linker")
        set(CMAKE_LINKER_TYPE MOLD)
    endif()

    # enable ccache if present
    find_program(CCACHE_FOUND ccache)
    if (CCACHE_FOUND)
        message(STATUS "Using ccache")
        set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing") # as recommended by Vulkan-Hpp
    if (STRICT_COMPILATION)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    endif()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-stringop-overflow -ffp-contract=off")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffp-model=precise")
		if (CMAKE_CXX_COMPILER_VERSION LESS 14 OR CROSS_PLATFORM_DETERMINISTIC)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffp-contract=off")
		endif()
	endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
endif()