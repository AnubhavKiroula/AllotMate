cmake_minimum_required(VERSION 3.16)

project(AllotMate
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(AllotMate
    src/main.cpp
)

target_include_directories(AllotMate
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(AllotMate PRIVATE /W4)
else()
    target_compile_options(AllotMate PRIVATE -Wall -Wextra)
endif()