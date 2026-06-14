cmake_minimum_required(VERSION 3.31)
project(carx LANGUAGES CXX)
project(carx VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)

find_package(Bullet REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(spdlog REQUIRED)
find_package(raylib REQUIRED)
find_package(GTest REQUIRED)
# find_package(SDL2 REQUIRED)

# find_package(bgfx REQUIRED)

# Рекурсивно находим все .cpp файлы в src/
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog raylib Bullet::Bullet gtest::gtest SDL2) # bgfx::bgfx)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src)

option(BUILD_TESTS "Build the tests" ON)
if(BUILD_TESTS)
    enable_testing()
    include(GoogleTest)
    add_subdirectory(tests)
endif()