cmake_minimum_required(VERSION 3.10)
project(hamza_web_framework)

# Find Git package for submodule handling
find_package(Git QUIET)

# Initialize and update git submodules to latest versions
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    option(GIT_SUBMODULE_UPDATE_LATEST "Update submodules to latest remote commits" ON)
    
    if(GIT_SUBMODULE)
        message(STATUS "Initializing and updating submodules...")
        
        # First, initialize submodules if they don't exist
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_INIT_RESULT)
        if(NOT GIT_SUBMOD_INIT_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_INIT_RESULT}, please checkout submodules")
        endif()
        
        # If enabled, update submodules to latest remote commits
        if(GIT_SUBMODULE_UPDATE_LATEST)
            message(STATUS "Updating submodules to latest remote commits...")
            execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --remote --recursive
                            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                            RESULT_VARIABLE GIT_SUBMOD_UPDATE_RESULT)
            if(NOT GIT_SUBMOD_UPDATE_RESULT EQUAL "0")
                message(WARNING "git submodule update --remote --recursive failed with ${GIT_SUBMOD_UPDATE_RESULT}, continuing with current submodule versions")
            else()
                message(STATUS "Submodules updated to latest versions successfully")
            endif()
        endif()
    endif()
endif()

# Check if submodules exist
set(REQUIRED_SUBMODULES "http-server" "html-builder")  # Add more submodules here as needed
foreach(submodule ${REQUIRED_SUBMODULES})
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/libs/${submodule}/CMakeLists.txt")
        message(FATAL_ERROR "The ${submodule} submodule was not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
    endif()
endforeach()

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
# Add include directories for all libraries
include_directories(${CMAKE_SOURCE_DIR}/includes)
include_directories(${CMAKE_SOURCE_DIR}/libs)

# Add include directories for each submodule
foreach(submodule ${REQUIRED_SUBMODULES})
    if(EXISTS "${PROJECT_SOURCE_DIR}/libs/${submodule}/includes")
        include_directories(${CMAKE_SOURCE_DIR}/libs/${submodule}/includes)
    endif()
endforeach()

# Add subdirectories for each submodule
foreach(submodule ${REQUIRED_SUBMODULES})
    add_subdirectory(libs/${submodule})
endforeach()

add_executable(hamza_web_framework app.cpp ${SRC_FILES})

# Link libraries for each submodule
set(SUBMODULE_LIBRARIES "http_server" "html_builder")  # Add more library names here as needed
target_link_libraries(hamza_web_framework ${SUBMODULE_LIBRARIES})