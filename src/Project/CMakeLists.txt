cmake_minimum_required(VERSION 3.14)
project(Project)

target_include_directories(imgui PUBLIC include)
target_include_directories(spdlog PUBLIC include)

set(sourceFiles
    Main.cpp
    ProjectApplication.cpp
)
set(headerFiles
    include/Project/ProjectApplication.hpp
)
add_executable(Project ${sourceFiles} ${headerFiles})

target_include_directories(Project PRIVATE include)
target_include_directories(Project.Library PUBLIC include)
target_link_libraries(Project PRIVATE glad imgui glm spdlog Project.Library)
