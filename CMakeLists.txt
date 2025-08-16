cmake_minimum_required(VERSION 3.10)
project(hamza_web_framework)


# define a variable called debug, that I can acess in a c++ code, to enable debugging
set(DEBUG_MODE ON CACHE BOOL "Enable Debug Mode") 

if(DEBUG_MODE)
    add_definitions(-DDEBUG_MODE)
endif()

# add debuging for c++ gcc

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -fsanitize=address -g -O0")


file(GLOB SRC_FILES src/*.cpp)

# link all cpp files inside framework folder, framework/src

set(CMAKE_CXX_STANDARD 17)
# Add include directory for header.hpp
include_directories(${CMAKE_SOURCE_DIR}/includes)
# Add include directory for http-server library headers
include_directories(${CMAKE_SOURCE_DIR}/libs/http-server/includes)

# Add the http-server library subdirectory
add_subdirectory(libs/http-server)

add_executable(  hamza_web_framework app.cpp ${SRC_FILES}  )

# Link the http_server library to the executable
target_link_libraries(hamza_web_framework http_server)