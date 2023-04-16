cmake_minimum_required(VERSION 3.14)
project(Project.Library)

add_subdirectory(lib)

set(sourceFiles
    Application.cpp
)

add_library(Project.Library ${sourceFiles})

find_package(OpenGL REQUIRED)

target_include_directories(Project.Library PUBLIC include)

target_link_libraries(Project.Library PRIVATE glfw glad glm TracyClient spdlog imgui)
