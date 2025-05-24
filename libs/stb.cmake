FetchContent_Declare(
    stb
    GIT_REPOSITORY  https://github.com/nothings/stb.git
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_MakeAvailable(stb)
    message("Fetching stb")

    add_library(stb
        INTERFACE ${stb_SOURCE_DIR}/stb_image.h
    )

    target_include_directories(stb
        INTERFACE ${stb_SOURCE_DIR}
    )
endif()
