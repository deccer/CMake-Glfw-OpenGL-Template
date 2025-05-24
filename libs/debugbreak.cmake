FetchContent_Declare(
    debugbreak
    GIT_REPOSITORY https://github.com/scottt/debugbreak
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_GetProperties(debugbreak)
if(NOT debugbreak_POPULATED)
    message("Fetching debugbreak")
    FetchContent_MakeAvailable(debugbreak)

    add_library(debugbreak
        INTERFACE ${debugbreak_SOURCE_DIR}/debugbreak.h
    )
    target_include_directories(debugbreak
        INTERFACE ${debugbreak_SOURCE_DIR}
    )
endif()
