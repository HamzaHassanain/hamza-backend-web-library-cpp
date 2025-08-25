# Simple C++ Backend Web Library

A simple C++ Web Framework for building some web applications. Built over the HTTP server I built here [hamza-http-server](https://github.com/HamzaHassanain/hamza-http-server-lib)

It also Uses these two libraries:

[hamza-json-parser](https://github.com/HamzaHassanain/hamza-json-parser) For JSON parsing and manipulation.

[hamza-html-builder](https://github.com/HamzaHassanain/hamza-html-builder) For building HTML responses.

## Table of Contents

- [Prerequisites](#prerequisites)

  - [For Linux (Ubuntu/Debian)](#for-linux-ubuntudebian)
  - [For Linux (CentOS/RHEL/Fedora)](#for-linux-centosrhelfedora)
  - [For Windows](#for-windows)

- [Quick Start](#quick-start)

  - [Using Git Submodules](#using-git-submodules)

- [Building the Project](#building-the-project)

  - [Step 1: Clone the Repository](#step-1-clone-the-repository)
  - [Step 2: Understanding Build Modes](#step-2-understanding-build-modes)
    - [Development Mode (WEB_LOCAL_TEST=1)](#development-mode-web_local_test1)
    - [Library Mode (WEB_LOCAL_TEST≠1)](#library-mode-web_local_test1)
  - [Step 3: Configure Build Mode](#step-3-configure-build-mode)
    - [For Development/Testing](#for-developmenttesting)
    - [For Library Distribution](#for-library-distribution)
  - [Step 4: Build the Project](#step-4-build-the-project)
    - [Option A: Using the Provided Script (Linux/Mac)](#option-a-using-the-provided-script-linuxmac)
    - [Option B: Manual Build (Linux/Mac/Windows)](#option-b-manual-build-linuxmacwindows)
    - [Windows-Specific Build Instructions](#windows-specific-build-instructions)
  - [Step 5: Run the Project](#step-5-run-the-project)
    - [Development Mode (WEB_LOCAL_TEST=1)](#development-mode-http_local_test1)
    - [Library Mode (WEB_LOCAL_TEST≠1)](#library-mode-http_local_test1)
  - [Using the Library in Your Own Project](#using-the-library-in-your-own-project)

  - [API Documentation](#api-documentation)

## Prerequisites

Before you start, ensure you have the following installed:

#### For Linux (Ubuntu/Debian):

```bash
# Update package list
sudo apt update

# Install essential build tools
sudo apt install build-essential cmake git

# Verify installations
gcc --version      # Should show GCC 7+ for C++17 support
cmake --version    # Should show CMake 3.10+
git --version      # Any recent version
```

#### For Linux (CentOS/RHEL/Fedora):

- CMake 3.10 or higher
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)

```bash
# For CentOS/RHEL
sudo yum groupinstall "Development Tools"
sudo yum install cmake git

# For Fedora
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake git
```

#### For Windows:

1. **Install Git**: Download from [git-scm.com](https://git-scm.com/download/win)
2. **Install CMake**: Download from [cmake.org](https://cmake.org/download/)
3. **Install a C++ Compiler** (choose one):
   - **Visual Studio 2019/2022** (recommended): Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/)
   - **MinGW-w64**: Download from [mingw-w64.org](https://www.mingw-w64.org/)
   - **MSYS2**: Download from [msys2.org](https://www.msys2.org/)

## Quick Start

### Using Git Submodules

You just need to clone the repository as a submodule:

```bash
# In your base project directory, run the following command
git submodule add https://github.com/HamzaHassanain/hamza-web-framework-cpp.git ./submodules/web-lib
```

Then in your project's CMakeLists.txt, include the submodule:

```cmake
# Your project's CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(my_project)

# This block checks for Git and initializes submodules recursively

# Enable submodule checking for all libraries
set(GIT_SUBMODULE ON CACHE BOOL "Enable submodule checking" FORCE)
set(GIT_SUBMODULE_UPDATE_LATEST ON CACHE BOOL "Enable submodule updates" FORCE)

# Disable submodule checking for all libraries.
# I recommend you uncomment these two lines once the building process is done for the first time
# Just comment it when you need to update to the latest version of the library 
# set(GIT_SUBMODULE OFF CACHE BOOL "Disable submodule checking" FORCE)
# set(GIT_SUBMODULE_UPDATE_LATEST OFF CACHE BOOL "Disable submodule updates" FORCE)


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


# Add the submodule
add_subdirectory(submodules/web-lib)

# Include directories for the library headers (this will also suppress warnings from headers)
target_include_directories(my_project SYSTEM PRIVATE submodules/web-lib/)

# Add additional compiler flags to suppress warnings from library headers
target_compile_options(my_project PRIVATE -Wno-comment -Wno-overloaded-virtual -Wno-reorder)

# Link against the main library
# The library's CMakeLists.txt handles linking all dependencies automatically
target_link_libraries(my_project hh_web_framework)
```

Then in your cpp file, include the http library header:

```cpp
#include "submodules/web-lib/web-lib.hpp" // to use the web library
#include "submodules/web-lib/libs/json/json-parser.hpp" // to use the json parser library
#include "submodules/web-lib/libs/html-builder/html-builder.hpp" // to use the html builder library
```

### Simple web server

```cpp
#include "submodules/web-lib/web-lib.hpp"
#include "submodules/web-lib/libs/json/json-parser.hpp"
#include "submodules/web-lib/libs/html-builder/html-builder.hpp"

int main() {
    hh_web::web_server server;

    int port = 3000;
    std::string host = "0.0.0.0";

    // Create server instance
    auto server = std::make_shared<hh_web::web_server<>>(port, host);

    server->start();
    return 0;
}
```

## Building The Project

### How to build

This guide will walk you through cloning, building, and running the project on both Linux and Windows systems.

### Step 1: Clone the Repository

Open your terminal (Linux) or Command Prompt/PowerShell (Windows):

```bash
# Clone the repository
git clone https://github.com/HamzaHassanain/hamza-web-framework-cpp.git

# Navigate to the project directory
cd hamza-web-framework-cpp

# Verify you're in the right directory
ls -la  # Linux/Mac
dir     # Windows CMD
```

### Step 2: Understanding Build Modes

The project supports two build modes controlled by the `.env` file:

#### Development Mode (WEB_LOCAL_TEST=1)

- Builds an **executable** for testing
- Includes debugging symbols and AddressSanitizer
- Perfect for learning and development

#### Library Mode (WEB_LOCAL_TEST≠1)

- Builds a **static library** for distribution
- Optimized for production use
- Other projects can link against it

### Step 3: Configure Build Mode

Create a `.env` file in the project root:

#### For Development/Testing

```bash
# Create .env file for development mode
echo "WEB_LOCAL_TEST=1" > .env

# On Windows (PowerShell):
echo "WEB_LOCAL_TEST=1" | Out-File -FilePath .env -Encoding ASCII

# On Windows (CMD):
echo WEB_LOCAL_TEST=1 > .env
```

#### For Library Distribution:

```bash
# Create .env file for library mode (or leave empty)
echo "WEB_LOCAL_TEST=0" > .env
```

### Step 4: Build the Project

#### Option A: Using the Provided Script (Linux/Mac)

```bash
# Make the script executable
chmod +x run.sh

# Run the build script
./run.sh
```

This script automatically:

1. Creates a `build` directory
2. Runs CMake configuration
3. Compiles the project
4. Runs the executable (if in development mode)

#### Option B: Manual Build (Linux/Mac/Windows)

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
cmake --build .

# Alternative: use make on Linux/Mac
make -j$(nproc)  # Linux
make -j$(sysctl -n hw.ncpu)  # Mac
```

#### Windows-Specific Build Instructions

**Using Visual Studio:**

```cmd
# Open Developer Command Prompt for Visual Studio
mkdir build
cd build
cmake ..
cmake --build . --config Release

```

**Using MinGW:**

```cmd
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

### Step 5: Run the Project

#### Development Mode (WEB_LOCAL_TEST=1):

```bash
# Linux/Mac
./build/hh_web_framework

# Windows
.\build\Debug\hh_web_framework.exe   # Debug build
.\build\Release\hh_web_framework.exe # Release build
```

#### Library Mode (WEB_LOCAL_TEST):

The build will create a static library file:

- **Linux/Mac**: `build/libhh_web_framework.a`
- **Windows**: `build/hh_web_framework.lib` or `build/Debug/hh_web_framework.lib`

### Using the Library in Your Own Project

Once built in library mode, you can use it in other projects:

```cmake
# Your project's CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(my_project)

# Find the library
find_library(WEB_LIB
    NAMES hh_web_framework
    PATHS path/to/hamza-web-framework-cpp/build
)

# Also You will need to link against the http-lib that this http server uses
find_library(HTTP_LIB
    NAMES http_lib
    PATHS path/to/hamza-web-framework-cpp/build/libs/http-lib/
)

# Also, Linkk The socket-lib that the http uses
find_library(SOCKET_LIB
    NAMES socket_lib
    PATHS path/to/hamza-web-framework-cpp/build/libs/http-lib/libs/socket-lib/
)

# Find the HTML builder library, Helps you build HTML templates
find_library(HTML_BUILDER_LIB
    NAMES html_builder
    PATHS path/to/hamza-web-framework-cpp/build/libs/html-builder/
)

# Find the JSON parser library, Helps you parse JSON data
find_library(JSON_PARSER_LIB
    NAMES json_parser
    PATHS path/to/hamza-web-framework-cpp/build/libs/json/
)

# include "path-to-hamza-hamza-web-framework-cpp/web-lib.hpp" in your cpp file for the full library
# or un-comment the bellow line
# target_include_directories(my_app PRIVATE /path/to/hamza-web-framework-cpp/includes)

# Link against the library
add_executable(my_app main.cpp)

target_link_libraries(my_app ${SOCKET_LIB})
target_link_libraries(my_app ${HTTP_LIB})
target_link_libraries(my_app ${HTML_BUILDER_LIB})
target_link_libraries(my_app ${JSON_PARSER_LIB})
target_link_libraries(my_app ${WEB_LIB})
target_link_libraries(my_app pthread) # For Linux/Mac
```

## API Documentation

Below is a reference of the core public classes and their commonly used methods. Include the corresponding header before use.

For detailed method signatures and advanced usage patterns, consult the comprehensive inline documentation in the header files located in `includes/` directory.

### hh_web::web_request

```cpp
#include "web_request.hpp"

// - Purpose: High-level wrapper for HTTP requests with enhanced functionality for web applications.
// - Key characteristics:
  // - Wraps lower-level HTTP request for simplified access
  // - Thread-safe path parameter extraction and manipulation
  // - Provides convenient access to common HTTP elements
  // - Designed as a base class that can be extended with virtual methods
// - Construction:
  web_request(hh_http::http_request &&req) // — takes ownership of HTTP request object
// - Request information (all methods are virtual and can be overridden):
  virtual std::string get_method() const // — retrieves HTTP method (GET, POST, etc.)
  virtual std::string get_path() const // — extracts path portion from URI
  virtual std::string get_uri() const // — returns complete request URI
  virtual std::string get_version() const // — returns HTTP version
  virtual std::string get_body() const // — returns request body content
// - Parameter extraction (all virtual):
  virtual std::vector<std::pair<std::string, std::string>> get_path_params() const // — extracts path parameters from URI
  virtual void set_path_params(const std::vector<std::pair<std::string, std::string>> &params) // — sets path parameters (used internally)
  virtual std::vector<std::pair<std::string, std::string>> get_query_parameters() const // — parses query string parameters
// - Header access (all virtual):
  virtual std::vector<std::string> get_header(const std::string &name) const // — retrieves values for specific header
  virtual std::vector<std::pair<std::string, std::string>> get_headers() const // — returns all request headers
// - Convenience methods (all virtual):
  virtual std::vector<std::string> get_content_type() const // — retrieves Content-Type header values
  virtual std::vector<std::string> get_cookies() const // — retrieves Cookie header values
  virtual std::vector<std::string> get_authorization() const // — retrieves Authorization header values
// - Extension points:
  // - All methods are virtual and can be overridden in derived classes
  // - Allows for custom request processing logic
  // - Thread-safety guaranteed in base implementation
```

### hh_web::web_response

```cpp
#include "web_response.hpp"

// - Purpose: High-level wrapper for HTTP responses with thread-safe sending and finalization.
// - Key characteristics:
  // - Wraps lower-level HTTP response for simplified usage
  // - Thread-safe header modification and response sending
  // - Prevents double-sending with atomic flags
  // - Automatic content-type setting for common response types
  // - Designed as a base class with virtual methods for customization
// - Construction:
  web_response(hh_http::http_response &&response) // — takes ownership of HTTP response object
// - Response configuration (all methods are virtual):
  virtual void set_status(int status_code, const std::string &status_message) // — sets HTTP status line
  virtual void set_body(const std::string &body) // — sets raw response content
  virtual void set_content_type(const std::string &content_type) // — sets Content-Type header
// - Header management (all virtual):
  virtual void add_header(const std::string &key, const std::string &value) // — adds HTTP header
  virtual void add_trailer(const std::string &key, const std::string &value) // — adds HTTP trailer
  virtual void add_cookie(const std::string &name, const std::string &cookie, const std::string &attributes = "") // — adds cookie with optional attributes
// - Response transmission (all virtual):
  virtual void send() noexcept // — finalizes and sends response (thread-safe, idempotent)
  virtual void send_json(const std::string &json_data) // — formats and sends JSON response
  virtual void send_html(const std::string &html_data) // — formats and sends HTML response
  virtual void send_text(const std::string &text_data) // — formats and sends plain text response
// - Lifecycle management:
  // - Thread-safe design prevents races in multi-threaded servers
  // - Automatic connection cleanup
  // - Exception handling during transmission
  // - All methods can be overridden to customize response behavior
```

### hh_web::web_route

```cpp
#include "web_route.hpp"

// - Purpose: Encapsulates a web route definition with path pattern matching and request handlers.
// - Key characteristics:
  // - Templated class with type parameters T (request) and G (response)
  // - Strong type safety with compile-time checks (static_assert)
  // - Handles path parameters extraction and matching
  // - Supports middleware chains with multiple handlers
  // - Provides virtual methods for customization in derived classes
// - Construction:
  web_route(const std::string &method, const std::string &expression, const std::vector<web_request_handler_t<T, G>> &handlers) // — creates route with HTTP method, path pattern, and handlers
// - Route information (all virtual):
  virtual std::string get_path() const // — returns the path expression/pattern
  virtual std::string get_method() const // — returns the HTTP method
// - Request processing (all virtual):
  virtual bool match(std::shared_ptr<T> request) const // — checks if request matches this route, extracts parameters
  virtual exit_code handle_request(std::shared_ptr<T> request, std::shared_ptr<G> response) const // — processes request through handlers chain
// - Template type requirements:
  // - T must derive from web_request (enforced with static_assert)
  // - G must derive from web_response (enforced with static_assert)
  // - Provides strong typing guarantees at compile time
// - Pattern matching features:
  // - Static path segments: "/api/users"
  // - Path parameters: "/api/users/:id"
  // - Automatic parameter extraction
  // - All core methods can be overridden for custom matching logic
```

### hh_web::web_router

```cpp
#include "web_router.hpp"

// - Purpose: Manages routes and middleware for web applications with request processing pipeline.
// - Key characteristics:
  // - Templated class with type parameters T (request) and G (response)
  // - Enforces type safety through static assertions
  // - Supports middleware chains for cross-cutting concerns
  // - Routes requests to appropriate handlers
  // - Comprehensive exception handling
  // - All key methods are virtual for extension
// - Construction:
  web_router() // — creates empty router with compile-time type safety validation
// - Router configuration (all virtual):
  virtual void register_route(std::shared_ptr<web_route<T, G>> route) // — adds new route to router
  virtual void register_middleware(const web_request_handler_t<T, G> &middleware) // — adds middleware handler
// - Request processing (all virtual):
  virtual bool handle_request(std::shared_ptr<T> request, std::shared_ptr<G> response) // — processes request through middleware and routes
  virtual exit_code middleware_handle_request(std::shared_ptr<T> request, std::shared_ptr<G> response) // — processes request through middleware only
// - Template type requirements:
  // - T must derive from web_request (enforced with static_assert)
  // - G must derive from web_response (enforced with static_assert)
  // - Provides strong typing guarantees at compile time
// - Design features:
  // - First-matching route execution
  // - Sequential middleware processing
  // - Automatic error conversion to HTTP responses
  // - Extensible through inheritance and method overriding
  // - Type-safe handler registration and execution
```

### hh_web::web_server

```cpp
#include "web_server.hpp"

// - Purpose: High-level web server template with multi-threaded request processing and routing.
// - Key characteristics:
  // - Templated class with type parameters T (request) and G (response)
  // - Multi-threaded architecture with worker thread pool
  // - Static file serving with automatic MIME type detection
  // - Comprehensive exception handling with proper HTTP responses
  // - Type safety enforced through static assertions
  // - All core methods are virtual for customization
// - Construction:
  web_server(uint16_t port, const std::string &host = "0.0.0.0") // — creates server instance listening on specified port/host
// - Server configuration (all virtual):
  virtual void register_router(std::shared_ptr<web_router<T, G>> router) // — adds a router for request handling
  virtual void register_static(const std::string &directory) // — registers directory for static file serving
  virtual void register_unmatched_route_handler(const web_request_handler_t<T, G> &handler) // — sets custom 404 handler
  virtual void register_headers_received_callback(const std::function<void(HEADER_RECEIVED_PARAMS)> &callback) // — sets callback for header processing
  virtual void register_unhandled_exception_callback(web_unhandled_exception_callback_t<T, G> callback) // — sets callback for unhandled exceptions
// - Server control (all virtual):
  virtual void listen(web_listen_callback_t listen_callback = nullptr, web_error_callback_t error_callback = nullptr) // — starts server with optional callbacks
  virtual void stop() // — stops server and terminates worker threads
// - Request processing (protected virtual methods):
  virtual void serve_static(std::shared_ptr<T> req, std::shared_ptr<G> res) // — serves static files with MIME type detection
  virtual void request_handler(std::shared_ptr<T> req, std::shared_ptr<G> res) // — main request processing pipeline
  virtual void on_request_received(hh_http::http_request &request, hh_http::http_response &response) override // — converts and dispatches HTTP requests
  virtual void on_unhandled_exception(std::shared_ptr<T> req, std::shared_ptr<G> res, const web_exception &e) // — handles uncaught exceptions
// - Template type requirements:
  // - T must derive from web_request (enforced with static_assert)
  // - G must derive from web_response (enforced with static_assert)
  // - Provides strong typing guarantees at compile time
// - Design features:
  // - Worker thread pool for high concurrency
  // - Static file serving with correct MIME types
  // - Automatic exception conversion to appropriate HTTP status codes
  // - Extensible through inheritance and method overriding
  // - Comprehensive callback system for server events
```

### hh_web::web_utilities

```cpp
#include "web_utilities.hpp"

// - Purpose: Collection of utility functions for HTTP and web server operations.
// - Key characteristics:
  // - Static file handling utilities
  // - URL encoding/decoding
  // - Path manipulation and validation
  // - MIME type detection
  // - Route pattern matching
// - Static file utilities:
  extern const std::vector<std::string> static_extensions // — list of known static file extensions
  extern const std::unordered_map<std::string, std::string> mime_types // — mapping of file extensions to MIME types
  std::string get_mime_type_from_extension(const std::string &extension) // — gets MIME type for a file extension
  std::string get_file_extension_from_uri(const std::string &uri) // — extracts file extension from URI
  bool is_uri_static(const std::string &uri) // — checks if URI points to a static resource
// - URL and path processing:
  std::string url_encode(const std::string &value) // — encodes string for URL inclusion (RFC 3986)
  std::string url_decode(const std::string &value) // — decodes percent-encoded URL string
  std::string sanitize_path(const std::string &path) // — removes dangerous sequences like ".." from paths
  std::string get_path(const std::string &uri) // — extracts path component from URI (strips query)
// - Parameter extraction:
  std::vector<std::pair<std::string, std::string>> get_query_parameters(const std::string &uri) // — parses query string parameters
  std::vector<std::pair<std::string, std::string>> get_path_params(const std::string &uri) // — extracts named parameters from path
// - Route matching:
  std::pair<bool, std::vector<std::pair<std::string, std::string>>> match_path(const std::string &expression, const std::string &rhs) // — matches route pattern against actual path
// - HTTP method validation:
  bool unknown_method(const std::string &method) // — checks if HTTP method is valid
// - Design features:
  // - Pure utility functions with no state
  // - Security-focused path handling
  // - Comprehensive MIME type support
  // - Efficient parameter extraction
```

### hh_web::web_types

```cpp
#include "web_types.hpp"

// - Purpose: Core type definitions and function signatures for the web framework.
// - Key characteristics:
  // - Defines function type aliases for callbacks and handlers
  // - Defines enum for request processing flow control
  // - Provides consistent type definitions across the framework
// - Core enum:
  enum class exit_code { EXIT = 1, CONTINUE = 0, ERROR = -1 } // — controls middleware/handler execution flow
// - Handler and callback types:
  using http_request_callback_t = std::function<void(hh_http::http_request &, hh_http::http_response &)> // — low-level HTTP handler
  using web_listen_callback_t = std::function<void()> // — server started callback
  using web_error_callback_t = std::function<void(const std::exception &)> // — server error callback
  template <typename T, typename G> using web_unhandled_exception_callback_t = std::function<void(std::shared_ptr<T>, std::shared_ptr<G>, const web_exception &)> // — exception handler
  template <typename T, typename G> using web_request_handler_t = std::function<exit_code(std::shared_ptr<T>, std::shared_ptr<G>)> // — main request handler type
// - Design features:
  // - Template-based callback definitions for type safety
  // - Consistent callback signatures throughout the framework
  // - Extensible through template parameters
```

### hh_web::web_methods

```cpp
#include "web_methods.hpp"

// - Purpose: Standard HTTP method constants and validation.
// - Key characteristics:
  // - Provides named constants for HTTP methods
  // - Used throughout the framework for method matching
// - HTTP method constants:
  extern const std::string GET // — HTTP GET method
  extern const std::string POST // — HTTP POST method
  extern const std::string PUT // — HTTP PUT method
  extern const std::string DELETE // — HTTP DELETE method
  extern const std::string PATCH // — HTTP PATCH method
  extern const std::string HEAD // — HTTP HEAD method
  extern const std::string OPTIONS // — HTTP OPTIONS method
// - Design features:
  // - Consistent method naming throughout codebase
  // - Used for route matching and method validation
```

### hh_web::logger

```cpp
#include "logger.hpp"

// - Purpose: Simple logging facility for the web framework.
// - Key characteristics:
  // - Thread-safe logging to files
  // - Multiple log levels (info, error, debug, trace, fatal)
  // - Configurable log path
  // - Can be enabled/disabled at runtime
// - Configuration:
  extern std::string absolute_path_to_logs // — path where log files will be created
  extern bool enabled_logging // — flag to enable/disable logging
// - Logging methods:
  void info(const std::string &message) // — logs informational message to info.log
  void error(const std::string &message) // — logs error message to error.log
  void debug(const std::string &message) // — logs debug message to debug.log
  void trace(const std::string &message) // — logs trace message to trace.log
  void fatal(const std::string &message) // — logs fatal message to fatal.log
// - Utility methods:
  void clear() // — clears all log files
// - Design features:
  // - Thread-safe with mutex protection
  // - Separate files for different log levels
  // - Minimal overhead when disabled
```
