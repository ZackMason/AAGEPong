cmake_minimum_required(VERSION 3.8.12)

project(game C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

if ( CMAKE_COMPILER_IS_GNUCC )
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()
if ( MSVC )
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W3 /wd4100 /wd4201")
endif()


add_subdirectory(ActuallyAGameEngine)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

file(GLOB_RECURSE src_files 
    ${PROJECT_SOURCE_DIR}/src/*.c*
)

add_compile_options(/bigobj)

include_directories(include)
include_directories(ActuallyAGameEngine/include)
add_executable(game ${src_files})

target_compile_definitions(game PUBLIC GAME_ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")
#target_compile_definitions(zenshi PUBLIC GAME_ASSETS_PATH="./assets/") # for release


file(GLOB_RECURSE test_files 
    ${PROJECT_SOURCE_DIR}/src/*.c*
    ${PROJECT_SOURCE_DIR}/tests/*.c*
)
list(REMOVE_ITEM test_files ${PROJECT_SOURCE_DIR}/src/main.c)
list(REMOVE_ITEM test_files ${PROJECT_SOURCE_DIR}/src/main.cpp)

add_executable(tests ${test_files})
target_compile_definitions(tests PUBLIC GAME_ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")
target_link_libraries(tests ${CONAN_LIBS} Engine)
target_link_libraries(game ${CONAN_LIBS} Engine)