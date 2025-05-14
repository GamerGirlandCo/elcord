cmake_minimum_required(VERSION 3.10)

# Project name and version
project(elcord-native VERSION 1.0 LANGUAGES CXX)
file(GLOB_RECURSE SRC "src/*.cpp")
#file(GLOB_RECURSE includes "C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC/Tools/MSVC/*")
include_directories("src/")
#set(CMAKE_GENERATOR_PLATFORM )
# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # does not produce the json file

#set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "") # works

# Add the executable

add_library(${PROJECT_NAME} SHARED ${SRC})

# Link against Windows libraries (if needed)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)
endif()
if(MSVC)
message("msvc?")
    target_compile_options(${PROJECT_NAME} PRIVATE "/Zc:__cplusplus" "/std:c++17")
endif()
