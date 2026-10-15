cmake_minimum_required(VERSION 3.20)

project(systemlens-backend VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(systemlens-backend
    src/main.cpp
)

if(MSVC)
    # Source files are UTF-8.
    target_compile_options(systemlens-backend PRIVATE /utf-8)
endif()
