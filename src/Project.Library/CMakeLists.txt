cmake_minimum_required(VERSION 3.14)
project(Project.Library)

add_subdirectory(lib)

set(sourceFiles
    Application.cpp
)

set(headerFiles
    include/Project.Library/Application.hpp
)
add_library(Project.Library ${sourceFiles} ${headerFiles})

find_package(OpenGL REQUIRED)

target_include_directories(glfw PUBLIC include)
target_include_directories(glad PUBLIC include)
target_include_directories(imgui PUBLIC include)
target_include_directories(spdlog PUBLIC include)
target_include_directories(Project.Library PRIVATE include)

target_link_libraries(Project.Library PRIVATE glfw glad glm TracyClient spdlog imgui)


