if(TARGET imgui)
    message("already fetched imgui")
else()
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG        docking
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        message("Fetching imgui")
        FetchContent_MakeAvailable(imgui)

        add_library(imgui
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp)

        target_include_directories(imgui
            PUBLIC ${imgui_SOURCE_DIR}
            PUBLIC ${imgui_SOURCE_DIR}/backends
            PUBLIC ${glfw_SOURCE_DIR}/include
        )

        target_link_libraries(imgui
            PRIVATE glfw
        )
    endif()
endif()
