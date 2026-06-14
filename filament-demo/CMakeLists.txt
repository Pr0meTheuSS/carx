cmake_minimum_required(VERSION 3.16)
project(filament_demo)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

set(FILAMENT_DIR ${CMAKE_SOURCE_DIR}/third_party/filament/)

# Добавляем пути к заголовкам
include_directories(
    ${FILAMENT_DIR}/include
)

# Указываем пути к библиотекам
link_directories(
    ${FILAMENT_DIR}/lib/x86_64
)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(app
    main.cpp
)

target_link_libraries(app PRIVATE
    filament
    filamat
    filaflat
    filabridge
    backend
    utils
    geometry

    smol-v   # ← ВАЖНО ЭТО


    SDL2

    bluegl
    bluevk
    vulkan
    glfw
    dl
    pthread
    OpenGL::GL
    X11
    xcb
    Xi
    rt
    zstd
)