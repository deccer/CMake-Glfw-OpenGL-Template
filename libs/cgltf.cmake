FetchContent_Declare(
    cgltf
    GIT_REPOSITORY  https://github.com/jkuhlmann/cgltf.git
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
FetchContent_GetProperties(cgltf)
if(NOT cgltf_POPULATED)
    message("Fetching cgltf")
    FetchContent_MakeAvailable(cgltf)

    add_library(cgltf
        INTERFACE ${cgltf_SOURCE_DIR}/cgltf.h
    )
    target_include_directories(cgltf
        INTERFACE ${cgltf_SOURCE_DIR}
    )
endif()