cmake_minimum_required(VERSION 3.28)
project(darling_editor C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT TARGET darling)
    include(FetchContent)
    FetchContent_Declare(
        darling
        GIT_REPOSITORY https://github.com/vexgraph-ecosystem/darling.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(darling)
endif()

file(GLOB_RECURSE DARLING_EDITOR_SOURCES
    "src/*.c"
)

if(DARLING_EDITOR_SOURCES)
    add_executable(darling_editor ${DARLING_EDITOR_SOURCES})
    target_include_directories(darling_editor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
            ../darling-editor
    )
    target_link_libraries(darling_editor PRIVATE darling)
else()
    add_custom_target(darling_editor)
endif()
