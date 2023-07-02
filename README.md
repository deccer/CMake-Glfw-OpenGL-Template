# CMake-Glfw-OpenGL-Template

## What is this?

CMake based C++ Project template with support for glfw, glad, imgui, spdlog, tracy, stbi, cgltf

This template is split into three parts

`Project` and `Project.Library` and `lib`

`Project` will be your actual project/game,
`Project.Library` containing supporting functions, things you might be reusing for something else. It could also contain your graphics/game engine abstractions.

This template provides a basic `Application` base class, ready to go, including window creation via `glfw`, hooking up `OpenGL` via `glad`, preparing UI rendering via `dear imgui`, logging via `spdlog` and comes with profiler capabilities via `tracy` too. The example also loads a model with textures via `cgltf` and `stbi`.

`lib` contains the list of dependencies used by projects across the solution, in this case, `imgui`, `spdlog`, `glm`, `tracy`, `stbi` and `cgltf`.

The project provides a tiny example of an application which most graphics-engine/game-engine/game developers/vfx artists or perhaps even researchers write, and its always the same, some window glue, some opengl loading,
some mesh rendering and texture loading, `spdlog` unifies logging here - as each and every project I've seen keeps using `printf` or `cout` or most likely some self-rolled `logger` which is really unnecessary.

![Screenshot](screenshots/screenshot.png)

## For whom is this for?

- Prototyping
- Game Jams
- Getting started with programming
- Getting started with graphics programming

## What do you need?

- `CMake`
- for VSCode the `CMake Tools` extension from Microsoft
- a compiler of your choice (Clang, GCC, MSVC)

Make sure to run `git submodule update --init --recursive` upon cloning the repository to ensure that Deccer's Cubes are included.

## Assumptions

This project template assumes the following things

- OpenGL 4.6 support (can be changed in `Application.cpp:59-60`)
- Resolution >= 1920x1080 so that you can actually use the window (can be changed in `Application.cpp:66-67`)

## What's next?

You most likely dont want to name your program `Project` and or `Project.Library`. Use your favorite search tool and replace `Project.Library` with `UE6.Engine` and `Project` with `UE6` :)
