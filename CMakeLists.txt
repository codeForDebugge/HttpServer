cmake_minimum_required(VERSION 3.10)
project(TCPServerProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Include header directory globally ---
include_directories(${CMAKE_SOURCE_DIR}/include)


# === Static Libraries ===
add_library(TCPServer STATIC src/TCPServer.cpp)
add_library(ThreadPool STATIC src/ThreadPool.cpp)

# === Main Executable ===
add_executable(server src/server.cpp)

# === Link Libraries ===
target_link_libraries(server PRIVATE TCPServer ThreadPool)


