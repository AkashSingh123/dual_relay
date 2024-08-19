cmake_minimum_required(VERSION 3.5)

project(RawDataSubscriber)

# Set the path to Fast DDS installation
set(fastdds_DIR "/usr/local/lib/cmake/fastdds")

# Find FastDDS
find_package(fastcdr REQUIRED)
find_package(fastrtps REQUIRED)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${FASTDDS_INCLUDE_DIR})

# Source files
set(SOURCES
    src/main.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} fastcdr fastrtps)

